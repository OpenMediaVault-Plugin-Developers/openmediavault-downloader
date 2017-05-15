/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2017 OpenMediaVault Plugin Developers
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
                "format"
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
            allowBlank    : false
        },{
            xtype         : "combo",
            name          : "format",
            fieldLabel    : _("Format"),
            allowBlank    : false,
            editable      : false,
            store         : [
                [ "aac", _("aac audio only") ],
                [ "m4a", _("m4a audio only") ],
                [ "mp3", _("mp3 audio only") ],
                [ "mp4", _("mp4/m4a") ],
                [ "wav", _("wav audio only") ],
                [ "webm", _("webm") ]
            ],
            mode          : "local",
            triggerAction : "all",
            value         : "mp4",
            hidden        : true
        },{
            xtype      : "textfield",
            name       : "url",
            fieldLabel : _("URL"),
            allowBlank : false
        },{
            xtype      : "sharedfoldercombo",
            name       : "sharedfolderref",
            fieldLabel : _("Shared Folder"),
            readOnly   : (me.uuid !== OMV.UUID_UNDEFINED),
            plugins    : [{
                ptype : "fieldinfo",
                text  : _("Downloads to this shared folder")
            }]
        },{
            xtype      : "checkbox",
            name       : "delete",
            fieldLabel : _("Delete"),
            checked    : false,
            boxLabel   : _("Delete from list of downloads after file is downloaded")
        }];
    }
});

Ext.define("OMV.module.admin.service.downloader.DownloadMultiple", {
    extend   : "OMV.workspace.window.Form",
    requires : [
        "OMV.workspace.window.plugin.ConfigObject"
    ],

    plugins: [{
        ptype : "configobject"
    }],

    rpcService   : "Downloader",
    rpcGetMethod : "getDownload",
    rpcSetMethod : "setDownload",

    width        : 600,

    getFormItems : function() {
        var me = this;
        return [{
            xtype         : "combo",
            name          : "format",
            fieldLabel    : _("Format"),
            allowBlank    : false,
            editable      : false,
            store         : [
                [ "aac", _("aac audio only") ],
                [ "m4a", _("m4a audio only") ],
                [ "mp3", _("mp3 audio only") ],
                [ "mp4", _("mp4/m4a") ],
                [ "wav", _("wav audio only") ],
                [ "webm", _("webm") ]
            ],
            mode          : "local",
            triggerAction : "all",
            value         : "mp4"
        },{
            xtype         : "textarea",
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
        },{
            xtype : "hiddenfield",
            name  : "dltype",
            value : "multiple"
        }];
    }
});

Ext.define("OMV.module.admin.service.downloader.Playlist", {
    extend   : "OMV.workspace.window.Form",
    requires : [
        "OMV.workspace.window.plugin.ConfigObject"
    ],

    plugins: [{
        ptype : "configobject"
    }],

    rpcService   : "Downloader",
    rpcGetMethod : "getDownload",
    rpcSetMethod : "setDownload",

    width        : 600,

    getFormItems : function() {
        var me = this;
        return [{
            xtype         : "combo",
            name          : "format",
            fieldLabel    : _("Format"),
            allowBlank    : false,
            editable      : false,
            store         : [
                [ "aac", _("aac audio only") ],
                [ "m4a", _("m4a audio only") ],
                [ "mp3", _("mp3 audio only") ],
                [ "mp4", _("mp4/m4a") ],
                [ "wav", _("wav audio only") ],
                [ "webm", _("webm") ]
            ],
            mode          : "local",
            triggerAction : "all",
            value         : "mp4"
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
        },{
            xtype : "hiddenfield",
            name  : "dltype",
            value : "playlist"
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
        "OMV.module.admin.service.downloader.Download",
        "OMV.module.admin.service.downloader.DownloadMultiple",
        "OMV.module.admin.service.downloader.Playlist"
    ],

    hidePagingToolbar : false,
    autoReload        : true,
    stateful          : true,
    stateId           : "a982a76d-6804-1632-a31b-8b48c0ea6dde",
    columns           : [{
        xtype     : "textcolumn",
        text      : _("Download Type"),
        sortable  : true,
        dataIndex : "dltype",
        stateId   : "dltype"
    },{
        xtype     : "textcolumn",
        text      : _("Filename"),
        sortable  : true,
        dataIndex : "filename",
        stateId   : "filename"
    },{
        xtype     : "templatecolumn",
        text      : _("URL"),
        sortable  : true,
        dataIndex : "url",
        stateId   : "url",
        tpl       : '<tpl for="url" between="&lt;br/&gt;">{.}</tpl>'
    },{
        xtype     : "textcolumn",
        text      : _("Shared Folder"),
        sortable  : true,
        dataIndex : "sharedfoldername",
        stateId   : "sharedfoldername"
    },{
        xtype     : "textcolumn",
        text      : _("Downloading"),
        sortable  : true,
        dataIndex : "downloading",
        stateId   : "downloading",
        renderer  : function (value) {
            var content;
            if ( value )
                content = _("Yes");
            else
                content = _("No");
            return content;
        }
    },{
        xtype     : "textcolumn",
        text      : _("Filesize"),
        sortable  : true,
        dataIndex : "filesize",
        stateId   : "filesize",
        renderer  : OMV.module.services.downloader.util.Format.fsRenderer
    },{
        xtype     : "textcolumn",
        text      : _("Delete after Download"),
        sortable  : true,
        dataIndex : "delete",
        stateId   : "delete",
        renderer  : function (value) {
            var content;
            if ( value )
                content = _("Yes");
            else
                content = _("No");
            return content;
        }
    },{
        xtype     : "textcolumn",
        text      : _("Format"),
        sortable  : true,
        dataIndex : "format",
        stateId   : "format"
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
                        { name : "format", type: "string" },
                        { name : "delete", type: "boolean" }
                    ]
                }),
                proxy    : {
                    type    : "rpc",
                    rpcData : {
                        service : "Downloader",
                        method  : "getDownloadList"
                    }
                }
            })
        });
        me.callParent(arguments);
    },

    getTopToolbarItems: function() {
        var me = this;
        var items = me.callParent(arguments);

        Ext.Array.insert(items, 1, [{
            id       : me.getId() + "-multiple",
            xtype    : "button",
            text     : _("Add Multiple"),
            icon     : "images/add.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : Ext.Function.bind(me.onMultipleButton, me, [ me ]),
            scope    : me
        },{
            id       : me.getId() + "-playlist",
            xtype    : "button",
            text     : _("Add Playlist"),
            icon     : "images/add.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : Ext.Function.bind(me.onPlaylistButton, me, [ me ]),
            scope    : me
        }]);

        Ext.Array.insert(items, 4, [{
            id       : me.getId() + "-download",
            xtype    : "button",
            text     : _("Download"),
            icon     : "images/download.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : Ext.Function.bind(me.onDownloadButton, me, [ me ]),
            scope    : me,
            disabled : true
        }]);

        Ext.Array.insert(items, 6, [{
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

    onMultipleButton : function() {
        var me = this;
        Ext.create("OMV.module.admin.service.downloader.DownloadMultiple", {
            title     : _("Add multiple downloads"),
            uuid      : OMV.UUID_UNDEFINED,
            listeners : {
                scope  : me,
                submit : function() {
                    this.doReload();
                }
            }
        }).show();
    },

    onPlaylistButton : function() {
        var me = this;
        Ext.create("OMV.module.admin.service.downloader.Playlist", {
            title     : _("Add playlist"),
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
        var type = record.get("dltype");
        var windowName = "";
        var title = "";

        switch (type) {
            case "playlist":
                windowName = "OMV.module.admin.service.downloader.Playlist";
                title = _("Edit playlist");
                break;
            case "multiple":
                windowName = "OMV.module.admin.service.downloader.DownloadMultiple";
                title = _("Edit multiple downloads");
                break;
            default:
                windowName = "OMV.module.admin.service.downloader.Download";
                title = _("Edit download");
        }

        Ext.create(windowName, {
            title     : _(title),
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
        var record = me.getSelected();
        OMV.Rpc.request({
            scope    : me,
            rpcData  : {
                service : "Downloader",
                method  : "doUpdate"
            }
        });
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
