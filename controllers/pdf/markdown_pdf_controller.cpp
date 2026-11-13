#include "controllers/pdf/markdown_pdf_controller.h"
#include "controllers/pdf/pdf_utils.h"

#include <QAbstractTextDocumentLayout>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QRegularExpression>
#include <QSet>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>
#include <QTextList>
#include <QTextTable>
#include <QUrl>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>
#include <cmath>
#include <vector>

namespace {

// QTextHtmlParser parses numeric CSS values (font-size, margin-*) using toInt().
// Convert px values (at 0.75 pt/px) and fractional pt values to integer pt so
// QTextHtmlParser never rejects a style property.
QString normalizeStyleUnitsToIntPt(const QString &style) {
  QString out = style;

  // 1. Convert px values on margin/padding/font-size to integer pt
  static const QRegularExpression pxRegex(
      QStringLiteral(R"((-?\d+(?:\.\d+)?)\s*px\b)"),
      QRegularExpression::CaseInsensitiveOption);
  {
    QRegularExpressionMatchIterator it = pxRegex.globalMatch(out);
    int offset = 0;
    while (it.hasNext()) {
      const QRegularExpressionMatch m = it.next();
      const double pxVal = m.captured(1).toDouble();
      const int ptVal = static_cast<int>(std::lround(pxVal * 0.75));
      const QString replacement = QStringLiteral("%1pt").arg(ptVal);
      out.replace(m.capturedStart() + offset, m.capturedLength(), replacement);
      offset += replacement.length() - m.capturedLength();
    }
  }

  // 2. Convert fractional pt values (e.g. 9.5pt, 10.5pt) to integer pt
  static const QRegularExpression fracPtRegex(
      QStringLiteral(R"((-?\d+\.\d+)\s*pt\b)"),
      QRegularExpression::CaseInsensitiveOption);
  {
    QRegularExpressionMatchIterator it = fracPtRegex.globalMatch(out);
    int offset = 0;
    while (it.hasNext()) {
      const QRegularExpressionMatch m = it.next();
      const double ptVal = m.captured(1).toDouble();
      const int intPt = static_cast<int>(std::lround(ptVal));
      const QString replacement = QStringLiteral("%1pt").arg(intPt);
      out.replace(m.capturedStart() + offset, m.capturedLength(), replacement);
      offset += replacement.length() - m.capturedLength();
    }
  }

  return out;
}

QString extractMarginStyle(const QString &rawStyle,
                           const QString &defaultTopPt = QStringLiteral("0pt"),
                           const QString &defaultBottomPt = QStringLiteral("0pt")) {
  const QString style = normalizeStyleUnitsToIntPt(rawStyle);
  QString topPt = defaultTopPt;
  QString bottomPt = defaultBottomPt;

  static const QRegularExpression topRe(
      QStringLiteral(R"(margin-top\s*:\s*([^;]+))"),
      QRegularExpression::CaseInsensitiveOption);
  static const QRegularExpression bottomRe(
      QStringLiteral(R"(margin-bottom\s*:\s*([^;]+))"),
      QRegularExpression::CaseInsensitiveOption);

  const QRegularExpressionMatch tm = topRe.match(style);
  if (tm.hasMatch()) {
    topPt = tm.captured(1).trimmed();
  }
  const QRegularExpressionMatch bm = bottomRe.match(style);
  if (bm.hasMatch()) {
    bottomPt = bm.captured(1).trimmed();
  }
  return QStringLiteral("margin-top: %1; margin-bottom: %2;")
      .arg(topPt, bottomPt);
}

QString formatInlineMarkdown(const QString &input) {
  QString text = input;

  // Step 1: Stash inline code spans `...`
  QStringList codeSpans;
  static const QRegularExpression codeRe(QStringLiteral(R"(`([^`]+)`)"));
  while (true) {
    const QRegularExpressionMatch m = codeRe.match(text);
    if (!m.hasMatch()) {
      break;
    }
    const QString escaped = m.captured(1).toHtmlEscaped();
    const QString htmlCode = QStringLiteral(
                                 "<code style=\"font-family: 'JetBrains Mono', "
                                 "'Consolas', monospace;\">%1</code>")
                                 .arg(escaped);
    const QString token = QStringLiteral("\x01CODE%1\x01").arg(codeSpans.size());
    codeSpans.append(htmlCode);
    text.replace(m.capturedStart(), m.capturedLength(), token);
  }

  // Step 2: Stash existing raw HTML tags <...> and normalize their style units
  QStringList htmlTags;
  static const QRegularExpression tagRe(
      QStringLiteral(R"(<[a-zA-Z/!][^>]*>)"));
  while (true) {
    const QRegularExpressionMatch m = tagRe.match(text);
    if (!m.hasMatch()) {
      break;
    }
    QString tag = m.captured(0);
    static const QRegularExpression styleAttrRe(
        QStringLiteral(R"re(style\s*=\s*"([^"]*)")re"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch sm = styleAttrRe.match(tag);
    if (sm.hasMatch()) {
      const QString updatedStyle = normalizeStyleUnitsToIntPt(sm.captured(1));
      tag.replace(sm.capturedStart(1), sm.capturedLength(1), updatedStyle);
    }
    const QString token = QStringLiteral("\x01TAG%1\x01").arg(htmlTags.size());
    htmlTags.append(tag);
    text.replace(m.capturedStart(), m.capturedLength(), token);
  }

  // Step 3: Convert Markdown links [label](url) and stash generated <a> tags
  static const QRegularExpression linkRe(
      QStringLiteral(R"(\[([^\]]+)\]\(([^)]+)\))"));
  while (true) {
    const QRegularExpressionMatch m = linkRe.match(text);
    if (!m.hasMatch()) {
      break;
    }
    const QString label = m.captured(1);
    const QString url = m.captured(2).trimmed();
    const QString openTag =
        QStringLiteral("<a href=\"%1\">").arg(url.toHtmlEscaped());
    const QString closeTag = QStringLiteral("</a>");
    const QString openToken =
        QStringLiteral("\x01TAG%1\x01").arg(htmlTags.size());
    htmlTags.append(openTag);
    const QString closeToken =
        QStringLiteral("\x01TAG%1\x01").arg(htmlTags.size());
    htmlTags.append(closeTag);
    const QString replacement = openToken + label + closeToken;
    text.replace(m.capturedStart(), m.capturedLength(), replacement);
  }

  // Step 4: Apply bold, italic, and strikethrough on tag-free text
  static const QRegularExpression boldItalicRe(
      QStringLiteral(R"(\*\*\*(.+?)\*\*\*)"));
  text.replace(boldItalicRe, QStringLiteral("<strong><em>\\1</em></strong>"));

  static const QRegularExpression boldRe(QStringLiteral(R"(\*\*(.+?)\*\*)"));
  text.replace(boldRe, QStringLiteral("<strong>\\1</strong>"));

  static const QRegularExpression italicRe(
      QStringLiteral(R"(\*([^*\n]+)\*)"));
  text.replace(italicRe, QStringLiteral("<em>\\1</em>"));

  static const QRegularExpression strikeRe(QStringLiteral(R"(~~(.+?)~~)"));
  text.replace(strikeRe, QStringLiteral("<s>\\1</s>"));

  // Step 5: Restore HTML tags and code spans
  for (int i = htmlTags.size() - 1; i >= 0; --i) {
    text.replace(QStringLiteral("\x01TAG%1\x01").arg(i), htmlTags.at(i));
  }
  for (int i = codeSpans.size() - 1; i >= 0; --i) {
    text.replace(QStringLiteral("\x01CODE%1\x01").arg(i), codeSpans.at(i));
  }

  return text;
}

QString transformFlexDivs(const QString &input) {
  static const QRegularExpression flexDivRe(
      QStringLiteral(
          R"re(<div\s+style="([^"]*display\s*:\s*flex[^"]*justify-content\s*:\s*space-between[^"]*)"\s*>\s*(<([a-zA-Z0-9]+)\b[^>]*>[\s\S]*?</\3>)\s*(<([a-zA-Z0-9]+)\b[^>]*>[\s\S]*?</\5>)\s*</div>)re"),
      QRegularExpression::CaseInsensitiveOption);

  QString out = input;
  while (true) {
    const QRegularExpressionMatch m = flexDivRe.match(out);
    if (!m.hasMatch()) {
      break;
    }
    const QString marginCss =
        extractMarginStyle(m.captured(1), QStringLiteral("0pt"), QStringLiteral("0pt"));
    const QString leftHtml = formatInlineMarkdown(m.captured(2).trimmed());
    const QString rightHtml = formatInlineMarkdown(m.captured(4).trimmed());

    // Emit as single-line table block so the line-by-line parser treats it atomically
    const QString tableHtml = QStringLiteral(
        "\n<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" "
        "style=\"%1 border: none;\"><tr>"
        "<td align=\"left\" valign=\"baseline\" style=\"padding: 0pt; border: none;\">%2</td>"
        "<td align=\"right\" valign=\"baseline\" style=\"padding: 0pt; border: none;\">%3</td>"
        "</tr></table>\n")
                                  .arg(marginCss, leftHtml, rightHtml);
    out.replace(m.capturedStart(), m.capturedLength(), tableHtml);
  }
  return out;
}

QString convertMarkdownToRichHtml(const QString &markdown, int baseFontPt) {
  QString preprocessed = markdown;
  preprocessed.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
  preprocessed.replace(QChar('\r'), QChar('\n'));

  // Convert flex space-between divs into 2-column borderless tables first
  preprocessed = transformFlexDivs(preprocessed);

  const QStringList lines = preprocessed.split(QChar('\n'));
  QStringList htmlOut;

  bool inCodeBlock = false;
  QStringList codeLines;
  bool inUl = false;
  bool inOl = false;
  QStringList paragraphLines;

  auto flushParagraph = [&]() {
    if (paragraphLines.isEmpty()) {
      return;
    }
    QString combined;
    for (int idx = 0; idx < paragraphLines.size(); ++idx) {
      if (idx > 0) {
        combined += QStringLiteral("<br/>");
      }
      combined += formatInlineMarkdown(paragraphLines.at(idx).trimmed());
    }
    htmlOut.append(
        QStringLiteral("<p style=\"margin-top: 1pt; margin-bottom: 3pt;\">%1</p>")
            .arg(combined));
    paragraphLines.clear();
  };

  auto closeLists = [&]() {
    if (inUl) {
      htmlOut.append(QStringLiteral("</ul>"));
      inUl = false;
    }
    if (inOl) {
      htmlOut.append(QStringLiteral("</ol>"));
      inOl = false;
    }
  };

  static const QRegularExpression headingRe(
      QStringLiteral(R"(^(#{1,6})\s+(.+)$)"));
  static const QRegularExpression hrRe(
      QStringLiteral(R"(^(?:---+|___+|\*\*\*+)\s*$)"));
  static const QRegularExpression ulRe(
      QStringLiteral(R"(^\s*[*+-]\s+(.+)$)"));
  static const QRegularExpression olRe(
      QStringLiteral(R"(^\s*\d+\.\s+(.+)$)"));
  static const QRegularExpression blockquoteRe(
      QStringLiteral(R"(^\s*>\s?(.*)$)"));

  int htmlBlockDepth = 0;

  for (int i = 0; i < lines.size(); ++i) {
    const QString &rawLine = lines.at(i);
    const QString trimmed = rawLine.trimmed();

    // Handle fenced code blocks
    if (trimmed.startsWith(QStringLiteral("```"))) {
      flushParagraph();
      closeLists();
      if (!inCodeBlock) {
        inCodeBlock = true;
        codeLines.clear();
      } else {
        inCodeBlock = false;
        htmlOut.append(
            QStringLiteral("<pre style=\"background-color: #f4f4f5; padding: "
                           "6pt; margin-top: 4pt; margin-bottom: 4pt; "
                           "font-family: 'JetBrains Mono', 'Consolas', "
                           "monospace;\">%1</pre>")
                .arg(codeLines.join(QChar('\n')).toHtmlEscaped()));
        codeLines.clear();
      }
      continue;
    }
    if (inCodeBlock) {
      codeLines.append(rawLine);
      continue;
    }

    // Multi-line raw HTML container tracking (e.g. <div style="text-align: center;"> ... </div>)
    if (htmlBlockDepth > 0) {
      const int opens = rawLine.count(
          QRegularExpression(QStringLiteral(R"(<div\b)"),
                             QRegularExpression::CaseInsensitiveOption));
      const int closes = rawLine.count(
          QRegularExpression(QStringLiteral(R"(</div\s*>)"),
                             QRegularExpression::CaseInsensitiveOption));
      htmlBlockDepth += (opens - closes);
      htmlOut.append(formatInlineMarkdown(rawLine));
      if (htmlBlockDepth < 0) {
        htmlBlockDepth = 0;
      }
      continue;
    }

    if (trimmed.isEmpty()) {
      flushParagraph();
      closeLists();
      continue;
    }

    // Standalone transformed <table> or block-level HTML element
    if (trimmed.startsWith(QStringLiteral("<table"), Qt::CaseInsensitive) ||
        trimmed.startsWith(QStringLiteral("<div"), Qt::CaseInsensitive) ||
        trimmed.startsWith(QStringLiteral("<h1"), Qt::CaseInsensitive) ||
        trimmed.startsWith(QStringLiteral("<h2"), Qt::CaseInsensitive) ||
        trimmed.startsWith(QStringLiteral("<h3"), Qt::CaseInsensitive) ||
        trimmed.startsWith(QStringLiteral("<p"), Qt::CaseInsensitive) ||
        trimmed.startsWith(QStringLiteral("<hr"), Qt::CaseInsensitive)) {
      flushParagraph();
      closeLists();
      const int opens = rawLine.count(
          QRegularExpression(QStringLiteral(R"(<div\b)"),
                             QRegularExpression::CaseInsensitiveOption));
      const int closes = rawLine.count(
          QRegularExpression(QStringLiteral(R"(</div\s*>)"),
                             QRegularExpression::CaseInsensitiveOption));
      if (opens > closes) {
        htmlBlockDepth = opens - closes;
      }
      htmlOut.append(formatInlineMarkdown(rawLine));
      continue;
    }

    // Horizontal rule
    if (hrRe.match(trimmed).hasMatch()) {
      flushParagraph();
      closeLists();
      htmlOut.append(QStringLiteral("<hr/>"));
      continue;
    }

    // Headings (# .. ######)
    const QRegularExpressionMatch hm = headingRe.match(trimmed);
    if (hm.hasMatch()) {
      flushParagraph();
      closeLists();
      const int level = hm.captured(1).length();
      const QString titleHtml = formatInlineMarkdown(hm.captured(2).trimmed());
      const int h1Pt =
          qMax(baseFontPt + 4, static_cast<int>(std::lround(baseFontPt * 1.6)));
      const int h2Pt =
          qMax(baseFontPt + 1, static_cast<int>(std::lround(baseFontPt * 1.1)));
      const int h3Pt =
          qMax(baseFontPt, static_cast<int>(std::lround(baseFontPt * 1.05)));
      if (level == 1) {
        htmlOut.append(
            QStringLiteral("<div style=\"font-size: %1pt; font-weight: bold; "
                           "color: #1F2937; margin-top: 6pt; margin-bottom: "
                           "2pt;\">%2</div>")
                .arg(h1Pt)
                .arg(titleHtml));
      } else if (level == 2) {
        htmlOut.append(
            QStringLiteral("<div style=\"font-size: %1pt; font-weight: bold; "
                           "color: #1F2937; margin-top: 5pt; margin-bottom: "
                           "0pt;\">%2</div><hr/>")
                .arg(h2Pt)
                .arg(titleHtml));
      } else if (level == 3) {
        htmlOut.append(
            QStringLiteral("<div style=\"font-size: %1pt; font-weight: bold; "
                           "color: #1F2937; margin-top: 4pt; margin-bottom: "
                           "1pt;\">%2</div>")
                .arg(h3Pt)
                .arg(titleHtml));
      } else {
        htmlOut.append(
            QStringLiteral("<div style=\"font-size: %1pt; font-weight: bold; "
                           "color: #1F2937; margin-top: 4pt; margin-bottom: "
                           "1pt;\">%2</div>")
                .arg(baseFontPt)
                .arg(titleHtml));
      }
      continue;
    }

    // Unordered list item
    const QRegularExpressionMatch ulm = ulRe.match(rawLine);
    if (ulm.hasMatch()) {
      flushParagraph();
      if (inOl) {
        htmlOut.append(QStringLiteral("</ol>"));
        inOl = false;
      }
      const QString itemContent = ulm.captured(1).trimmed();
      if (!inUl) {
        htmlOut.append(QStringLiteral(
            "<ul type=\"disc\" style=\"margin-top: 1pt; margin-bottom: 2pt; "
            "list-style-type: disc;\">"));
        inUl = true;
      }
      htmlOut.append(
          QStringLiteral(
              "<li style=\"margin-top: 0pt; margin-bottom: 1pt;\">%1</li>")
              .arg(formatInlineMarkdown(itemContent)));
      continue;
    }

    // Ordered list item
    const QRegularExpressionMatch olm = olRe.match(rawLine);
    if (olm.hasMatch()) {
      flushParagraph();
      if (inUl) {
        htmlOut.append(QStringLiteral("</ul>"));
        inUl = false;
      }
      if (!inOl) {
        htmlOut.append(QStringLiteral(
            "<ol style=\"margin-top: 1pt; margin-bottom: 2pt;\">"));
        inOl = true;
      }
      htmlOut.append(
          QStringLiteral(
              "<li style=\"margin-top: 0pt; margin-bottom: 1pt;\">%1</li>")
              .arg(formatInlineMarkdown(olm.captured(1).trimmed())));
      continue;
    }

    // Blockquote
    const QRegularExpressionMatch bqm = blockquoteRe.match(rawLine);
    if (bqm.hasMatch()) {
      flushParagraph();
      closeLists();
      htmlOut.append(
          QStringLiteral("<blockquote style=\"color: #52525b; margin-left: "
                         "10pt; margin-top: 2pt; margin-bottom: 4pt;\">%1</blockquote>")
              .arg(formatInlineMarkdown(bqm.captured(1).trimmed())));
      continue;
    }

    // Markdown table (| col1 | col2 |)
    if (trimmed.startsWith(QChar('|')) && trimmed.endsWith(QChar('|')) &&
        trimmed.size() > 2) {
      flushParagraph();
      closeLists();
      QStringList tableRows;
      while (i < lines.size()) {
        const QString rowLine = lines.at(i).trimmed();
        if (rowLine.startsWith(QChar('|')) && rowLine.endsWith(QChar('|')) &&
            rowLine.size() > 2) {
          tableRows.append(rowLine);
          ++i;
        } else {
          break;
        }
      }
      --i;

      static const QRegularExpression sepRowRe(
          QStringLiteral(R"(^\|[\s:|-]+\|$)"));
      htmlOut.append(QStringLiteral(
          "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"4\" "
          "style=\"border-collapse: collapse; margin-top: 4pt; margin-bottom: 6pt;\">"));
      bool isHeaderRow =
          (tableRows.size() >= 2 && sepRowRe.match(tableRows.at(1)).hasMatch());
      for (int r = 0; r < tableRows.size(); ++r) {
        if (r == 1 && isHeaderRow) {
          continue;
        }
        const QString inner =
            tableRows.at(r).mid(1, tableRows.at(r).length() - 2);
        const QStringList cells = inner.split(QChar('|'));
        htmlOut.append(QStringLiteral("<tr>"));
        for (const QString &cell : cells) {
          const QString cellHtml = formatInlineMarkdown(cell.trimmed());
          if (r == 0 && isHeaderRow) {
            htmlOut.append(QStringLiteral(
                               "<th align=\"left\" style=\"border: 1px solid #d4d4d8; "
                               "background-color: #f4f4f5; font-weight: bold; "
                               "padding: 4pt 6pt;\">%1</th>")
                               .arg(cellHtml));
          } else {
            htmlOut.append(QStringLiteral(
                               "<td align=\"left\" style=\"border: 1px solid #d4d4d8; "
                               "padding: 4pt 6pt;\">%1</td>")
                               .arg(cellHtml));
          }
        }
        htmlOut.append(QStringLiteral("</tr>"));
      }
      htmlOut.append(QStringLiteral("</table>"));
      continue;
    }

    // Standard paragraph or inline HTML continuation (e.g. <span ...>)
    closeLists();
    paragraphLines.append(rawLine);
  }

  flushParagraph();
  closeLists();

  return QStringLiteral("<!DOCTYPE html><html><body>%1</body></html>")
      .arg(htmlOut.join(QChar('\n')));
}

void polishDocumentLayout(QTextDocument &doc, double basePt) {
  // Compact <hr/> blocks and normalize <h2> section headings
  for (QTextBlock block = doc.begin(); block.isValid(); block = block.next()) {
    const QTextBlockFormat bf = block.blockFormat();
    if (bf.hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth)) {
      QTextCursor cursor(block);
      QTextBlockFormat updatedBf = bf;
      updatedBf.setTopMargin(1.5);
      updatedBf.setBottomMargin(3.0);
      cursor.setBlockFormat(updatedBf);

      QTextCharFormat cf = cursor.blockCharFormat();
      cf.setFontPointSize(1.0);
      cursor.setBlockCharFormat(cf);
    } else if (bf.headingLevel() == 2) {
      QTextCursor cursor(block);
      QTextBlockFormat updatedBf = bf;
      updatedBf.setTopMargin(6.0);
      updatedBf.setBottomMargin(0.0);
      cursor.setBlockFormat(updatedBf);

      cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
      QTextCharFormat cf;
      cf.setFontPointSize(qMax(basePt + 1.0, std::round(basePt * 1.1)));
      cf.setFontWeight(QFont::Bold);
      cursor.mergeCharFormat(cf);
    }
  }
}

struct PageSlice {
  double topY{0.0};
  double height{0.0};
};

std::vector<PageSlice> computePageSlices(const QTextDocument &doc,
                                         double pageHeightPt) {
  struct Span {
    double top{0.0};
    double bottom{0.0};
  };

  std::vector<Span> atomicSpans;
  QAbstractTextDocumentLayout *docLayout = doc.documentLayout();
  if (!docLayout) {
    return {{0.0, pageHeightPt}};
  }

  bool mergeWithPrevious = false;
  for (QTextBlock block = doc.begin(); block.isValid(); block = block.next()) {
    const QRectF br = docLayout->blockBoundingRect(block);
    const QTextBlockFormat bf = block.blockFormat();
    const bool isHr =
        bf.hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth);
    const bool isSectionHeading =
        !isHr && block.blockNumber() > 3 && block.next().isValid() &&
        block.next().blockFormat().hasProperty(
            QTextFormat::BlockTrailingHorizontalRulerWidth);

    if (isHr) {
      if (!atomicSpans.empty() && mergeWithPrevious) {
        atomicSpans.back().bottom =
            std::max(atomicSpans.back().bottom, br.bottom());
      } else {
        atomicSpans.push_back({br.top(), br.bottom()});
      }
      // Keep mergeWithPrevious active so the block right after a section's <hr/>
      // stays attached to the section heading + <hr/>.
      continue;
    }

    const QTextLayout *tl = block.layout();
    if (tl && tl->lineCount() > 0) {
      for (int i = 0; i < tl->lineCount(); ++i) {
        const QTextLine line = tl->lineAt(i);
        const double lTop = br.top() + line.y();
        const double lBottom = lTop + line.height();
        if (mergeWithPrevious && !atomicSpans.empty()) {
          atomicSpans.back().bottom =
              std::max(atomicSpans.back().bottom, lBottom);
        } else {
          atomicSpans.push_back({lTop, lBottom});
        }
      }
    } else if (br.height() > 0.0) {
      if (mergeWithPrevious && !atomicSpans.empty()) {
        atomicSpans.back().bottom =
            std::max(atomicSpans.back().bottom, br.bottom());
      } else {
        atomicSpans.push_back({br.top(), br.bottom()});
      }
    }

    mergeWithPrevious = isSectionHeading;
  }

  const double totalHeight = doc.size().height();
  if (totalHeight <= 0.0) {
    return {{0.0, pageHeightPt}};
  }

  std::vector<PageSlice> slices;
  double currentTop = 0.0;

  while (currentTop + 1.0 < totalHeight) {
    double cutY = currentTop + pageHeightPt;
    if (cutY >= totalHeight - 0.5) {
      slices.push_back({currentTop, totalHeight - currentTop});
      break;
    }

    // Move cutY up so it never slices through any atomic line/ruler span
    bool adjusted = true;
    int cutSpanIdx = -1;
    while (adjusted) {
      adjusted = false;
      for (size_t idx = 0; idx < atomicSpans.size(); ++idx) {
        const Span &s = atomicSpans[idx];
        if (s.top < cutY - 0.1 && s.bottom > cutY + 0.1 &&
            s.top > currentTop + 1.0) {
          cutY = s.top;
          cutSpanIdx = static_cast<int>(idx);
          adjusted = true;
        }
      }
    }

    if (cutSpanIdx > 0) {
      const double prevBottom = atomicSpans[cutSpanIdx - 1].bottom;
      if (prevBottom < cutY && prevBottom > currentTop + 1.0) {
        cutY = 0.5 * (prevBottom + cutY);
      }
    }

    if (cutY <= currentTop + 1.0) {
      cutY = currentTop + pageHeightPt;
    }

    slices.push_back({currentTop, cutY - currentTop});
    currentTop = cutY;
  }

  if (slices.empty()) {
    slices.push_back({0.0, pageHeightPt});
  }
  return slices;
}

} // namespace

MarkdownPdfController::MarkdownPdfController(QObject *parent)
    : PdfToolController(parent) {
  qRegisterMetaType<MarkdownPdfController::Result>();
}

void MarkdownPdfController::start(const Params &params) {
  waitForRunningJob();
  m_isCancelled = false;
  QFuture<void> future = QtConcurrent::run([this, params]() {
    runJob(params);
  });
  m_watcher.setFuture(future);
}

void MarkdownPdfController::runJob(const Params &params) {
  Result result;
  result.outputDir = params.outputDir;

  const QStringList &sourcePaths = params.sourcePaths;
  const int totalFiles = sourcePaths.size();
  const int clampedMarginPt = qBound(0, params.marginPt, 144);
  const int clampedFontPt = qBound(6, params.baseFontPt, 36);

  emit progressChanged(0, totalFiles, "Rendering documents to PDF...");

  int pdfsCreated = 0;
  int failedFiles = 0;
  QStringList errors;
  QSet<QString> usedFileNames;

  const QString defaultCss = QStringLiteral(
      "body { color: #111827; }"
      "a { color: #2563eb; text-decoration: none; }"
      "pre, code { font-family: 'JetBrains Mono', 'Fira Code', 'Consolas', monospace; }");

  for (int i = 0; i < totalFiles; ++i) {
    if (m_isCancelled) {
      for (int rem = i; rem < totalFiles; ++rem) {
        emit fileStatusChanged(sourcePaths.at(rem),
                               static_cast<int>(FileStatus::Cancelled),
                               "PDF export cancelled");
      }
      break;
    }

    const QString &sourcePath = sourcePaths.at(i);
    const QFileInfo sourceInfo(sourcePath);

    emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Processing),
                           "Typesetting document to PDF");
    emit progressChanged(
        i, totalFiles,
        QString("Exporting %1 (%2 of %3)...")
            .arg(sourceInfo.fileName())
            .arg(i + 1)
            .arg(totalFiles));

    QFile inFile(sourceInfo.absoluteFilePath());
    if (!inFile.open(QIODevice::ReadOnly)) {
      failedFiles++;
      const QString err =
          QString("Cannot open file: %1").arg(inFile.errorString());
      errors.append(QString("%1: %2").arg(sourceInfo.fileName(), err));
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed),
                             err);
      continue;
    }
    const QByteArray rawBytes = inFile.readAll();
    inFile.close();

    const QString content = QString::fromUtf8(rawBytes);

    const QString baseStem = sourceInfo.completeBaseName().isEmpty()
                                 ? QStringLiteral("document")
                                 : sourceInfo.completeBaseName();
    QString uniqueStem = baseStem;
    int suffix = 1;
    while (true) {
      const QString candidateName = uniqueStem + ".pdf";
      const QString candidatePath =
          QDir(params.outputDir).filePath(candidateName);
      const QFileInfo candidateInfo(candidatePath);
      const bool collidesWithSource =
          pdfutil::isSamePdf(sourceInfo, candidateInfo);
      if (!usedFileNames.contains(candidateName.toLower()) &&
          !QFile::exists(candidatePath) && !collidesWithSource) {
        break;
      }
      uniqueStem = QString("%1_%2").arg(baseStem).arg(suffix++);
    }
    const QString outFileName = uniqueStem + ".pdf";
    usedFileNames.insert(outFileName.toLower());
    const QString outPdfPath = QDir(params.outputDir).filePath(outFileName);

    const QString tempPath =
        outPdfPath + QString(".md_tmp_%1_%2")
                         .arg(QCoreApplication::applicationPid())
                         .arg(QDateTime::currentMSecsSinceEpoch());

    QString fileError;
    {
      QPdfWriter writer(tempPath);
      writer.setResolution(72);
      writer.setTitle(sourceInfo.completeBaseName());
      writer.setPageSize(
          QPageSize(params.useLetterPageSize ? QPageSize::Letter : QPageSize::A4));
      writer.setPageOrientation(params.landscape ? QPageLayout::Landscape
                                                 : QPageLayout::Portrait);
      writer.setPageMargins(QMarginsF(clampedMarginPt, clampedMarginPt,
                                      clampedMarginPt, clampedMarginPt),
                            QPageLayout::Point);

      const QSizeF pageSizePt = writer.pageLayout().paintRectPoints().size();

      QTextDocument doc;
      if (doc.documentLayout()) {
        doc.documentLayout()->setPaintDevice(&writer);
      }
      doc.setBaseUrl(QUrl::fromLocalFile(sourceInfo.absolutePath() + "/"));
      QFont font("Inter", clampedFontPt);
      doc.setDefaultFont(font);
      doc.setDocumentMargin(0.0);
      doc.setIndentWidth(10.0);
      doc.setDefaultStyleSheet(defaultCss);

      const QString ext = sourceInfo.suffix().toLower();
      if (ext == "html" || ext == "htm") {
        doc.setHtml(transformFlexDivs(content));
      } else {
        doc.setHtml(convertMarkdownToRichHtml(content, clampedFontPt));
      }

      polishDocumentLayout(doc, clampedFontPt);
      doc.setTextWidth(pageSizePt.width());

      const std::vector<PageSlice> slices =
          computePageSlices(doc, pageSizePt.height());

      QPainter painter(&writer);
      if (!painter.isActive()) {
        fileError = "Failed to initialize PDF painter.";
      } else {
        for (size_t p = 0; p < slices.size(); ++p) {
          if (m_isCancelled) {
            break;
          }
          if (p > 0) {
            writer.newPage();
          }
          const PageSlice &slice = slices[p];
          const double topInset = (p > 0) ? 0.5 : 0.0;
          const double bottomInset = (p + 1 < slices.size()) ? 0.5 : 0.0;
          const QRectF clipRect(0.0, slice.topY + topInset, pageSizePt.width(),
                                qMax(1.0, slice.height - topInset - bottomInset));
          painter.save();
          painter.translate(0.0, -slice.topY);
          doc.drawContents(&painter, clipRect);
          painter.restore();
        }
        painter.end();
      }
    }

    if (!m_isCancelled && fileError.isEmpty()) {
      QPdfDocument verifyDoc;
      if (verifyDoc.load(tempPath) != QPdfDocument::Error::None ||
          verifyDoc.pageCount() <= 0) {
        fileError = "Generated PDF failed validation.";
      } else {
        verifyDoc.close();
        if (!pdfutil::savePdfAtomically(tempPath, outPdfPath, m_isCancelled,
                                        fileError)) {
          if (fileError.isEmpty() && !m_isCancelled) {
            fileError = "Failed to save PDF.";
          }
        }
      }
    }
    QFile::remove(tempPath);

    if (m_isCancelled) {
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Cancelled),
                             "PDF export cancelled");
      for (int rem = i + 1; rem < totalFiles; ++rem) {
        emit fileStatusChanged(sourcePaths.at(rem),
                               static_cast<int>(FileStatus::Cancelled),
                               "PDF export cancelled");
      }
      break;
    }

    if (fileError.isEmpty()) {
      pdfsCreated++;
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Succeeded),
                             QString("Saved %1").arg(outFileName));
    } else {
      failedFiles++;
      errors.append(QString("%1: %2").arg(sourceInfo.fileName(), fileError));
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed),
                             fileError);
    }

    emit progressChanged(
        i + 1, totalFiles,
        QString("Exported %1 of %2 document(s)").arg(i + 1).arg(totalFiles));
  }

  result.success = (pdfsCreated > 0 && failedFiles == 0 && !m_isCancelled);
  result.cancelled = m_isCancelled.load();
  result.pdfsCreated = pdfsCreated;
  result.failedFiles = failedFiles;
  result.errors = errors;
  if (!errors.isEmpty()) {
    result.error = errors.join("\n");
  }

  emit finished(result);
}
