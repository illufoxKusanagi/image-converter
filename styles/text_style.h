#ifndef TEXTSTYLE_H
#define TEXTSTYLE_H

#include <QString>

namespace TextStyle {
inline QString Heading1() {
  return "font-family: 'Roboto'; font-size: 48px; font-weight: 600;";
}
inline QString Heading2() {
  return "font-family: 'Roboto'; font-size: 40px; font-weight: 600;";
}
inline QString Heading3() {
  return "font-family: 'Roboto'; font-size: 32px; font-weight: 600;";
}
inline QString Heading4() {
  return "font-family: 'Roboto'; font-size: 28px; font-weight: 600;";
}
inline QString Heading5() {
  return "font-family: 'Roboto'; font-size: 24px; font-weight: 600;";
}
inline QString BodyBigBold() {
  return "font-family: 'Roboto'; font-size: 18px; font-weight: 600; ";
}
inline QString BodyBigRegular() {
  return "font-family: 'Roboto'; font-size: 18px; font-weight: 400; ";
}
inline QString BodyMediumBold() {
  return "font-family: 'Roboto'; font-size: 16px; font-weight: 600; ";
}
inline QString BodyMediumRegular() {
  return "font-family: 'Roboto'; font-size: 16px; font-weight: 400; ";
}
inline QString BodySmallBold() {
  return "font-family: 'Roboto'; font-size: 14px; font-weight: 600; ";
}
inline QString BodySmallRegular() {
  return "font-family: 'Roboto'; font-size: 14px; font-weight: 400; ";
}
inline QString SubttileBig() {
  return "font-family: 'Roboto'; font-size: 14px; font-weight: 400; ";
}
inline QString SubttileBigBold() {
  return "font-family: 'Roboto'; font-size: 14px; font-weight: 600; ";
}
inline QString SubttileMediumRegular() {
  return "font-family: 'Roboto'; font-size: 12px; font-weight: 400; ";
}
inline QString SubttileMediumBold() {
  return "font-family: 'Roboto'; font-size: 12px; font-weight: 600; ";
}
inline QString SubttileSmallBold() {
  return "font-family: 'Roboto'; font-size: 10px; font-weight: 600; ";
}
inline QString SubttileSmallRegular() {
  return "font-family: 'Roboto'; font-size: 10px; font-weight: 400; ";
}
} // namespace TextStyle

#endif // TEXTSTYLE_H
