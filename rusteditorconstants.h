/*
 *  RustEditor: Plugin to add Rust language support to QtCreator IDE.
 *  Copyright (C) 2015  Davide Ghilardi
 *
 *  This file is part of RustEditor.
 *
 *  RustEditor is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  RustEditor is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with RustEditor.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RUSTEDITORCONSTANTS_H
#define RUSTEDITORCONSTANTS_H

namespace RustEditor {
namespace Constants {

const char RUSTEDITOR_ID[] = "RustEditor.RustEditor";
const char RUSTEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Rust Editor");
const char RUST_SOURCE_MIMETYPE[] = "text/x-rustsrc";

const char RUSTEDITOR_SETTINGS_ID[] = "Rust.Configurations";
const char RUSTEDITOR_SETTINGS_CATEGORY[] = "Rust";
const char RUSTEDITOR_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("Rust", "Rust");
const char RUSTEDITOR_SETTINGS_CATEGORY_ICON[] = ":/rusteditor/images/QtRust.png";

} // namespace RustEditor
} // namespace Constants

#endif // RUSTEDITORCONSTANTS_H

