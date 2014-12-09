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
// require("js/omv/form/field/SharedFolderComboBox.js")
// require("js/omv/form/plugin/LinkedFields.js")
// require("js/omv/module/admin/service/downloader/util/Format.js")

Ext.define("OMV.module.admin.service.downloader.Download", {
    extend   : "OMV.workspace.window.Form",
    requires : [
        "OMV.workspace.window.plugin.ConfigObject",
        "OMV.form.plugin.LinkedFields"
    ],

    plugins: [{
        ptype : "configobject"
    },{
        ptype        : "linkedfields",
        correlations : [{
            conditions : [{
                name  : "dltype",
                value : "youtube-dl"
            }],
            name       : [
                "keepvideo"
            ],
            properties : [
                "show"
            ]
        }]
    }],

    rpcService   : "Downloader",
    rpcGetMethod : "getDownload",
    rpcSetMethod : "setDownload",

    width        : 600,

    getFormItems : function() {
        var me = this;
        return [{
            xtype         : "combo",
            name          : "dltype",
            fieldLabel    : _("Download Type"),
            allowBlank    : false,
            editable      : false,
            store         : [
                [ "aria2", _("aria2") ],
                [ "curl", _("curl") ],
                [ "youtube-dl", _("youtube-dl") ]
            ],
            mode          : "local",
            triggerAction : "all",
            value         : ""
        },{
            xtype         : "textfield",
            name          : "filename",
            fieldLabel    : _("Filename"),
            allowBlank    : false,
            plugins       : [{
                ptype : "fieldinfo",
                text  : _("Saves download as this filename. If file extension is aac, m4a, mp3, or wav, audio will be extracted to filename.")
            }]
        },{
            xtype      : "checkbox",
            name       : "keepvideo",
            fieldLabel : _("Keep Video"),
            boxLabel   : _("Keep video file after conversion to audio.  Video will have mp4 extension."),
            checked    : false,
            hidden     : true
        },{
            xtype         : "textfield",
            name          : "url",
            fieldLabel    : _("URL"),
            allowBlank    : false
        },{
            xtype         : "sharedfoldercombo",
            name          : "sharedfolderref",
            fieldLabel    : _("Shared Folder"),
            readOnly      : (me.uuid !== OMV.UUID_UNDEFINED),
            plugins       : [{
                ptype : "fieldinfo",
                text  : _("Downloads to this shared folder")
            }]
        },{
            xtype         : "checkbox",
            name          : "delete",
            fieldLabel    : _("Delete"),
            checked       : false,
            boxLabel      : _("Delete from list of downloads after file is downloaded")
        }];
    }
});

Ext.define("OMV.module.admin.service.downloader.Downloads", {
    extend   : "OMV.workspace.grid.Panel",
    requires : [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc",
        "OMV.util.Format"
    ],
    uses     : [
        "OMV.module.admin.service.downloader.Download"
    ],

    hidePagingToolbar : false,
    autoReload        : true,
    stateful          : true,
    stateId           : "a982a76d-6804-1632-a31b-8b48c0ea6dde",
    columns           : [{
        text        : _("Download Type"),
        sortable    : true,
        dataIndex   : "dltype",
        stateId     : "dltype"
    },{
        text        : _("Filename"),
        sortable    : true,
        dataIndex   : "filename",
        stateId     : "filename"
    },{
        text        : _("URL"),
        sortable    : true,
        dataIndex   : "url",
        stateId     : "url"
    },{
        text        : _("Shared Folder"),
        sortable    : true,
        dataIndex   : "sharedfoldername",
        stateId     : "sharedfoldername"
    },{
        text        : _("Downloading"),
        sortable    : true,
        dataIndex   : "downloading",
        stateId     : "downloading",
        renderer    : function (value) {
            var content;
            if ( value )
                content = _("Yes");
            else
                content = _("No");
            return content;
        }
    },{
        text      : _("Filesize"),
        sortable  : true,
        dataIndex : "filesize",
        stateId   : "filesize",
        renderer  : OMV.module.services.downloader.util.Format.fsRenderer
    },{
        text        : _("Delete after Download"),
        sortable    : true,
        dataIndex   : "delete",
        stateId     : "delete",
        renderer    : function (value) {
            var content;
            if ( value )
                content = _("Yes");
            else
                content = _("No");
            return content;
        }
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
                        { name : "dltype", type: "string" },
                        { name : "filename", type: "string" },
                        { name : "url", type: "string" },
                        { name : "sharedfoldername", type: "string" },
                        { name : "downloading", type: "boolean" },
                        { name : "filesize", type: "string" },
                        { name : "delete", type: "boolean" }
                    ]
                }),
                proxy    : {
                    type    : "rpc",
                    rpcData : {
                        service : "Downloader",
                        method  : "getDownloads"
                    }
                }
            })
        });
        me.callParent(arguments);
    },

    getTopToolbarItems: function() {
        var me = this;
        var items = me.callParent(arguments);

        Ext.Array.insert(items, 2, [{
            id       : me.getId() + "-download",
            xtype    : "button",
            text     : _("Download"),
            icon     : "images/download.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : Ext.Function.bind(me.onDownloadButton, me, [ me ]),
            scope    : me,
            disabled : true
        }]);

        Ext.Array.insert(items, 4, [{
            id       : me.getId() + "-update",
            xtype    : "button",
            text     : _("Update youtube-dl"),
            icon     : "images/refresh.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : Ext.Function.bind(me.onUpdateButton, me, [ me ]),
            scope    : me
        }]);

        return items;
    },

    onSelectionChange: function(model, records) {
        var me = this;
        var download = true;
        me.callParent(arguments);
        if(records.length == 1) {
            var record = me.getSelected();
            if(record.get("downloading") == false) {
                download = false;
            }
        }
        me.setToolbarButtonDisabled("download", download);
    },

    onAddButton : function() {
        var me = this;
        Ext.create("OMV.module.admin.service.downloader.Download", {
            title     : _("Add download"),
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
        Ext.create("OMV.module.admin.service.downloader.Download", {
            title     : _("Edit download"),
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
                method  : "deleteDownload",
                params  : {
                    uuid : record.get("uuid")
                }
            }
        });
    },

    onDownloadButton: function() {
        var me = this;
        var record = me.getSelected();
        OMV.Rpc.request({
            scope    : me,
            rpcData  : {
                service : "Downloader",
                method  : "doDownload",
                params  : {
                    uuid : record.get("uuid")
                }
            }
        });
        me.doReload();
        me.setToolbarButtonDisabled("download", true);
    },

    onUpdateButton: function() {
        var me = this;
        var wnd = Ext.create("OMV.window.Execute", {
            title      : _("Update youtube-dl"),
            rpcService : "Downloader",
            rpcMethod  : "doUpdate",
            hideStartButton : true,
            hideStopButton  : true,
            listeners       : {
                scope     : me,
                finish    : function(wnd, response) {
                    wnd.appendValue(_("Done..."));
                    wnd.setButtonDisabled("close", false);
                },
                exception : function(wnd, error) {
                    OMV.MessageBox.error(null, error);
                    wnd.setButtonDisabled("close", false);
                }
            }
        });
        wnd.setButtonDisabled("close", true);
        wnd.show();
        wnd.start();
    }
});

OMV.WorkspaceManager.registerNode({
    id      : "downloader",
    path    : "/service",
    text    : _("Downloader"),
    icon16  : "images/downloader.png",
    iconSvg : "images/downloader.svg"
});

OMV.WorkspaceManager.registerPanel({
    id        : "downloads",
    path      : "/service/downloader",
    text      : _("Downloads"),
    position  : 10,
    className : "OMV.module.admin.service.downloader.Downloads"
});
