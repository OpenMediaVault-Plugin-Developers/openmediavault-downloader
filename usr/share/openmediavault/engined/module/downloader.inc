<?php
/*
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
require_once("openmediavault/module.inc");
require_once("openmediavault/config.inc");
require_once("openmediavault/error.inc");
require_once("openmediavault/util.inc");
require_once("openmediavault/log.inc");

class OMVModuleDownloader extends OMVModuleServiceAbstract implements OMVINotifyListener
{
    public function getName()
    {
        return "downloader";
    }
}

// Register module
$moduleMgr = &OMVModuleMgr::getInstance();
$moduleMgr->registerModule(new OMVModuleDownloader());

// Register new logfile type
OMVLogFile::registerType("downloader", array(
    "filename" => "downloader.log",
    "filepath" => "/var/log/downloader.log",
    "regex"    => "/^\[((.*?)\s+(.*?))\]\s+(.*?):\s+(.*?):\s+(.*?):\s+(.*?):\s+(.*?):\s+(.*?)$/",
    "columns"  => array(
        "date"      => 1,
        "component" => 4,
        "type"      => 5,
        "filename"  => 6,
        "folder"    => 7,
        "message"   => 8,
        "url"       => 9
    )
));
