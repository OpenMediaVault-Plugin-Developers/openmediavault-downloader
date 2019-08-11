/**
 *
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2019 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/form/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/form/field/SharedFolderComboBox.js")
// require("js/omv/form/field/UserComboBox.js")

Ext.define("OMV.module.admin.service.downloader.Upload", {
    extend : "OMV.workspace.form.Panel",
    uses   : [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc"
    ],

    rpcService      : "Downloader",
    rpcGetMethod    : "getUpload",
    rpcSetMethod    : "setUpload",

    hideOkButton  : true,

    getFormItems    : function() {
        var me = this;
        return [{
            xtype    : "fieldset",
            title    : _("Settings"),
            defaults : {
                labelSeparator : ""
            },
            items : [{
                xtype      : "sharedfoldercombo",
                name       : "uploadref",
                fieldLabel : _("Shared Folder"),
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Upload file to this shared folder.")
                }]
            },{
                xtype      : "usercombo",
                name       : "username",
                fieldLabel : _("File Owner"),
                value      : "root"
            },{
                xtype   : "button",
                name    : "upload",
                text    : _("Upload"),
                scope   : this,
                handler : Ext.Function.bind(me.onUploadButton, me, [ me ]),
                margin  : "0 0 5 0"
            }]
        }];
    },

    onUploadButton : function() {
        var me = this;
        me.doSubmit();
        Ext.create("OMV.window.Upload", {
            title     : _("Upload file"),
            service   : "Downloader",
            method    : "doUpload",
            listeners : {
                scope   : me,
                success : function(wnd, response) {
                    me.doReload();
                }
            }
        }).show();
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "upload",
    path      : "/service/downloader",
    text      : _("Upload"),
    position  : 20,
    className : "OMV.module.admin.service.downloader.Upload"
});
