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

#ifndef RUSTEDITOR_H
#define RUSTEDITOR_H

#include "rusteditor_global.h"

#include <extensionsystem/iplugin.h>

namespace RustEditor {
namespace Internal {

class RustEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "RustEditor.json")

public:
    RustEditorPlugin();
    ~RustEditorPlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();
};

} // namespace Internal
} // namespace RustEditor

#endif // RUSTEDITOR_H

