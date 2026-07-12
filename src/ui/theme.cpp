/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ui/theme.h"

#include <windows/win/dark.h>

#include "base/format.h"
#include "base/gfx.h"
#include "base/string.h"
#include "base/xml.h"
#include "taiga/config.h"
#include "taiga/path.h"
#include "ui/ui.h"

namespace ui {

// Light palette defaults; swapped by InitDarkColors()
COLORREF kColorDarkBlue = RGB(46, 81, 162);
COLORREF kColorGray = RGB(230, 230, 230);
COLORREF kColorLightBlue = RGB(225, 231, 245);
COLORREF kColorLightGray = RGB(248, 248, 248);
COLORREF kColorLightGreen = RGB(225, 245, 231);
COLORREF kColorLightRed = RGB(245, 225, 231);
COLORREF kColorMainInstruction = RGB(0x00, 0x33, 0x99);

void InitDarkColors() {
  if (!win::dark::Enabled())
    return;

  kColorDarkBlue = RGB(0x4C, 0x8B, 0xD8);         // stats chart bars
  kColorGray = RGB(0x39, 0x40, 0x4B);             // card borders
  kColorLightBlue = RGB(0x2C, 0x3B, 0x52);        // "finished airing" band
  kColorLightGray = RGB(0x2B, 0x31, 0x3A);        // card backgrounds
  kColorLightGreen = RGB(0x27, 0x3D, 0x30);       // "airing" band, now playing
  kColorLightRed = RGB(0x45, 0x2B, 0x33);         // "not yet aired" band
  kColorMainInstruction = RGB(0x6C, 0xB3, 0xF0);  // header text
}

ThemeManager::ThemeManager() {
  icons16_.Create(ScaleX(16), ScaleY(16));  // 16px
  icons24_.Create(ScaleX(24), ScaleY(24));  // 24px
}

bool ThemeManager::Load() {
  XmlDocument document;
  const auto path = taiga::GetPath(taiga::Path::ThemeCurrent);
  const auto parse_result = XmlLoadFileToDocument(document, path);

  if (!parse_result) {
    ui::DisplayErrorMessage(L"Could not read theme file:\n" + path, TAIGA_APP_NAME);
    return false;
  }

  auto node_icons16 =
      document.child(L"theme").child(L"icons").child(L"set_16px");
  auto node_icons24 =
      document.child(L"theme").child(L"icons").child(L"set_24px");
  auto node_image =
      document.child(L"theme").child(L"list").child(L"background").child(L"image");
  auto node_progress =
      document.child(L"theme").child(L"list").child(L"progress");

  // Icons
  std::vector<std::wstring> icons16;
  for (auto node_icon : node_icons16.children(L"icon")) {
    icons16.push_back(node_icon.attribute(L"name").value());
  }
  std::vector<std::wstring> icons24;
  for (auto node_icon : node_icons24.children(L"icon")) {
    icons24.push_back(node_icon.attribute(L"name").value());
  }

  // List
  #define READ_PROGRESS_DATA(x, name) \
      list_progress_[x].type = node_progress.child(name).attribute(L"type").value(); \
      list_progress_[x].value[0] = HexToARGB(node_progress.child(name).attribute(L"value_1").value()); \
      list_progress_[x].value[1] = HexToARGB(node_progress.child(name).attribute(L"value_2").value()); \
      list_progress_[x].value[2] = HexToARGB(node_progress.child(name).attribute(L"value_3").value());
  READ_PROGRESS_DATA(kListProgressAired, L"aired");
  READ_PROGRESS_DATA(kListProgressAvailable, L"available");
  READ_PROGRESS_DATA(kListProgressBackground, L"background");
  READ_PROGRESS_DATA(kListProgressBorder, L"border");
  READ_PROGRESS_DATA(kListProgressButton, L"button");
  READ_PROGRESS_DATA(kListProgressCompleted, L"completed");
  READ_PROGRESS_DATA(kListProgressDropped, L"dropped");
  READ_PROGRESS_DATA(kListProgressOnHold, L"onhold");
  READ_PROGRESS_DATA(kListProgressPlanToWatch, L"plantowatch");
  READ_PROGRESS_DATA(kListProgressSeparator, L"separator");
  READ_PROGRESS_DATA(kListProgressWatching, L"watching");
  #undef READ_PROGRESS_DATA

  // Load icons
  icons16_.Remove(-1);
  icons24_.Remove(-1);
  const auto theme_path = GetPathOnly(taiga::GetPath(taiga::Path::ThemeCurrent));
  HBITMAP bitmap_handle = nullptr;
  for (size_t i = 0; i < kIconCount16px && i < icons16.size(); i++) {
    bitmap_handle =
        LoadImage(L"{}16px\\{}.png"_format(theme_path, icons16.at(i)),
                  ScaleX(16), ScaleY(16));
    icons16_.AddBitmap(bitmap_handle, CLR_NONE);
    DeleteObject(bitmap_handle);
  }
  for (size_t i = 0; i < kIconCount24px && i < icons24.size(); i++) {
    bitmap_handle =
        LoadImage(L"{}24px\\{}.png"_format(theme_path, icons24.at(i)),
                  ScaleX(24), ScaleY(24));
    icons24_.AddBitmap(bitmap_handle, CLR_NONE);
    DeleteObject(bitmap_handle);
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

void ThemeManager::CreateBrushes() {
  if (brush_background_.Get())
    return;

  brush_background_.Set(CreateSolidBrush(win::dark::SysColor(COLOR_WINDOW)));
}

void ThemeManager::CreateFonts(HDC hdc) {
  if (font_bold_.Get() && font_header_.Get())
    return;

  LOGFONT lFont = {0};
  lFont.lfCharSet = DEFAULT_CHARSET;
  lFont.lfOutPrecision = OUT_STRING_PRECIS;
  lFont.lfClipPrecision = CLIP_STROKE_PRECIS;
  lFont.lfQuality = PROOF_QUALITY;
  lFont.lfPitchAndFamily = VARIABLE_PITCH;

  // Bold font
  lFont.lfHeight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
  lFont.lfWeight = FW_BOLD;
  lstrcpy(lFont.lfFaceName, L"Segoe UI");
  font_bold_.Set(::CreateFontIndirect(&lFont));

  // Header font
  lFont.lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
  lFont.lfWeight = FW_NORMAL;
  lstrcpy(lFont.lfFaceName, L"Segoe UI");
  font_header_.Set(::CreateFontIndirect(&lFont));
}

HBRUSH ThemeManager::GetBackgroundBrush() const {
  return brush_background_.Get();
}

HFONT ThemeManager::GetBoldFont() const {
  return font_bold_.Get();
}

HFONT ThemeManager::GetHeaderFont() const {
  return font_header_.Get();
}

win::ImageList& ThemeManager::GetImageList16() {
  return icons16_;
}

win::ImageList& ThemeManager::GetImageList24() {
  return icons24_;
}

void ThemeManager::DrawListProgress(HDC hdc, const LPRECT rect,
                                    ListProgressType type) {
  auto& item = list_progress_[type];

  // Solid
  if (item.type == L"solid") {
    HBRUSH hbrSolid = CreateSolidBrush(item.value[0]);
    FillRect(hdc, rect, hbrSolid);
    DeleteObject(hbrSolid);

  // Gradient
  } else if (item.type == L"gradient") {
    GradientRect(hdc, rect, item.value[0], item.value[1], item.value[2] > 0);

  // Progress bar
  } else if (item.type == L"progress") {
    DrawProgressBar(hdc, rect, item.value[0], item.value[1], item.value[2]);
  }
}

COLORREF ThemeManager::GetListProgressColor(ListProgressType type) {
  return list_progress_[type].value[0];
}

}  // namespace ui
