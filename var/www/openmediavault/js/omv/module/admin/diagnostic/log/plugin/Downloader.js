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
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// require("js/omv/PluginManager.js")
// require("js/omv/module/admin/diagnostic/log/plugin/Plugin.js")
// require("js/omv/util/Format.js")

Ext.define("OMV.module.admin.diagnostic.log.plugin.Downloader", {
    extend : "OMV.module.admin.diagnostic.log.plugin.Plugin",
    alias  : "omv.plugin.diagnostic.log.downloader",

    id       : "downloader",
    text     : _("Downloader"),
    stateful : true,
    stateId  : "41adf493-a76a-481e-b3c0-43cb368c97c0",
    columns  : [{
        text      : _("Date & Time"),
        sortable  : true,
        dataIndex : "date",
        stateId   : "date",
        flex      : 1
    },{
        text      : _("Component"),
        sortable  : true,
        dataIndex : "component",
        stateId   : "component",
        flex      : 1
    },{
        text      : _("Type"),
        sortable  : true,
        dataIndex : "type",
        stateId   : "type",
        flex      : 1
    },{
        text      : _("Filename"),
        sortable  : true,
        dataIndex : "filename",
        stateId   : "filename",
        flex      : 1
    },{
        text      : _("URL"),
        sortable  : true,
        dataIndex : "url",
        stateId   : "url",
        flex      : 1
    },{
        text      : _("Shared Folder"),
        sortable  : true,
        dataIndex : "folder",
        stateId   : "folder",
        flex      : 1
    },{
        text      : _("Message"),
        sortable  : true,
        dataIndex : "message",
        stateId   : "message",
        flex      : 1
    }],
    rpcParams : {
        id : "downloader"
    },
    rpcFields : [
        { name : "date", type : "string" },
        { name : "component", type : "string" },
        { name : "type", type : "string" },
        { name : "filename", type : "string" },
        { name : "url", type : "string" },
        { name : "folder", type : "string" },
        { name : "message", type : "string" }
    ]
});
