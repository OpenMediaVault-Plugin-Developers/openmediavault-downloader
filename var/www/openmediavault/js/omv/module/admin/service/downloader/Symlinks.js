/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2014 OpenMediaVault Plugin Developers
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
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/util/Format.js")
// require("js/omv/Rpc.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/window/RootFolderBrowser.js")

Ext.define("OMV.module.admin.service.downloader.Symlink", {
    extend   : "OMV.workspace.window.Form",
    requires : [
        "OMV.workspace.window.plugin.ConfigObject"
    ],
    uses: [
        "OMV.data.Model",
        "OMV.data.Store",
        "OMV.window.RootFolderBrowser"
    ],

    rpcService   : "Downloader",
    rpcGetMethod : "getSymlink",
    rpcSetMethod : "setSymlink",
    plugins      : [{
        ptype : "configobject"
    }],

    width        : 600,

    getFormItems : function() {
        var me = this;
        return [{
            xtype      : "checkbox",
            name       : "enable",
            fieldLabel : _("Enable"),
            checked    : true
        },{
            xtype          : "textfield",
            name           : "source",
            fieldLabel     : _("Source"),
            allowBlank     : false,
            triggers       : {
                folder : {
                    cls     : Ext.baseCSSPrefix + "form-folder-trigger",
                    handler : "onTriggerClick"
                }
            },
            onTriggerClick : function() {
                Ext.create("OMV.window.RootFolderBrowser", {
                    listeners : {
                        scope  : this,
                        select : function(wnd, node, path) {
                            // Set the selected path.
                            this.setValue(path);
                        }
                    }
                }).show();
            }
        },{
            xtype          : "textfield",
            name           : "destination",
            fieldLabel     : _("Destination"),
            allowBlank     : false,
            triggers       : {
                folder : {
                    cls     : Ext.baseCSSPrefix + "form-folder-trigger",
                    handler : "onTriggerClick"
                }
            },
            onTriggerClick : function() {
                Ext.create("OMV.window.RootFolderBrowser", {
                    listeners : {
                        scope  : this,
                        select : function(wnd, node, path) {
                            // Set the selected path.
                            this.setValue(path);
                        }
                    }
                }).show();
            }
        }];
    }
});

Ext.define("OMV.module.admin.service.downloader.Symlinks", {
    extend   : "OMV.workspace.grid.Panel",
    requires : [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc",
        "OMV.util.Format"
    ],
    uses     : [
        "OMV.module.admin.service.downloader.Symlink"
    ],

    hidePagingToolbar : false,
    stateful          : true,
    stateId           : "a982a76d-6804-1632-b31b-8b48c0ea6dde",
    columns           : [{
        xtype     : "booleaniconcolumn",
        text      : _("Enabled"),
        sortable  : true,
        dataIndex : "enable",
        stateId   : "enable",
        align     : "center",
        width     : 80,
        resizable : false,
        trueIcon  : "switch_on.png",
        falseIcon : "switch_off.png"
    },{
        text      : _("Source"),
        sortable  : true,
        dataIndex : "source",
        stateId   : "source"
    },{
        text      : _("Destination"),
        sortable  : true,
        dataIndex : "destination",
        stateId   : "destination"
    }],

    initComponent: function() {
        var me = this;
        Ext.apply(me, {
            store : Ext.create("OMV.data.Store", {
                autoLoad : true,
                model    : OMV.data.Model.createImplicit({
                    idProperty : "uuid",
                    fields     : [
                        { name : "uuid", type: "string" },
                        { name : "enable", type: "boolean" },
                        { name : "source", type: "string" },
                        { name : "destination", type: "string" }
                    ]
                }),
                proxy    : {
                    type    : "rpc",
                    rpcData : {
                        service : "Downloader",
                        method  : "getSymlinks"
                    }
                }
            })
        });
        me.callParent(arguments);
    },

    onAddButton : function() {
        var me = this;
        Ext.create("OMV.module.admin.service.downloader.Symlink", {
            title     : _("Add symlink"),
            uuid      : OMV.UUID_UNDEFINED,
            listeners : {
                scope  : me,
                submit : function() {
                    this.doReload();
                }
            }
        }).show();
    },

    onEditButton : function() {
        var me = this;
        var record = me.getSelected();
        Ext.create("OMV.module.admin.service.downloader.Symlink", {
            title     : _("Edit symlink"),
            uuid      : record.get("uuid"),
            listeners : {
                scope  : me,
                submit : function() {
                    this.doReload();
                }
            }
        }).show();
    },

    doDeletion : function(record) {
        var me = this;
        OMV.Rpc.request({
            scope    : me,
            callback : me.onDeletion,
            rpcData  : {
                service : "Downloader",
                method  : "deleteSymlink",
                params  : {
                    uuid : record.get("uuid")
                }
            }
        });
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "symlinks",
    path      : "/service/downloader",
    text      : _("Symlinks"),
    position  : 30,
    className : "OMV.module.admin.service.downloader.Symlinks"
});
