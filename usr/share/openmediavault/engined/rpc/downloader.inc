<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2016 OpenMediaVault Plugin Developers
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
require_once("openmediavault/globals.inc");
require_once("openmediavault/functions.inc");

class OMVRpcServiceDownloader extends \OMV\Rpc\ServiceAbstract
{
    private $getAria2     = "/usr/bin/aria2c";
    private $getCurl      = "/usr/bin/curl";
    private $getLogFile   = "/var/log/downloader.log";
    private $getPidDir    = "/var/run/";
    private $getYoutubedl = "/usr/sbin/youtube-dl";

    public function getName()
    {
        return "Downloader";
    }

    public function initialize()
    {
        $this->registerMethod("getDownloadList");
        $this->registerMethod("getDownload");
        $this->registerMethod("setDownload");
        $this->registerMethod("deleteDownload");
        $this->registerMethod("doDownload");

        $this->registerMethod("getUpload");
        $this->registerMethod("setUpload");
        $this->registerMethod("doUpload");
        $this->registerMethod("doUpdate");
    }

    public function getDownloadList($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.common.getlist");
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $objects = $db->get("conf.service.downloader.download");
        // Add additional share informations.
        $objectsAssoc = [];

        foreach ($objects as $objectk => &$objectv) {
            // Add new properties
            $objectv->add('downloading', 'boolean', false);
            $objectv->add('filesize', 'integer', 0);
            $objectv->add("sharedfoldername", "string", gettext("n/a"));

            // Get the shared folder configuration object and build path
            if (!empty($objectv->get("sharedfolderref"))) {
                $sfObject = $db->get("conf.system.sharedfolder", $objectv->get("sharedfolderref"));
                $objectv->set('sharedfoldername', $sfObject->get("name"));
                // Get the mount point configuration object to build the absolute shared folder path.
                $meObject = $db->get("conf.system.filesystem.mountpoint", $sfObject->get("mntentref"));
                // Return the absolute shared folder path.
                $sfPath = build_path(DIRECTORY_SEPARATOR, $meObject->get("dir"), $sfObject->get("reldirpath"));
            } else {
                continue;
            }
            $filename = sprintf("%s/%s", $sfPath, $objectv->get("filename"));
            $filenamepart = sprintf("%s/%s.part", $sfPath, $objectv->get("filename"));
            $filenamearia = sprintf("%s/%s.aria2", $sfPath, $objectv->get("filename"));
            $pid = sprintf("%sdownloader-%s.pid", $this->getPidDir, $objectv->get('uuid'));

            if (file_exists($pid)) {
                $objectv->set('downloading', true);

                switch ($objectv->get('dltype')) {
                    case "curl":
                        $objectv->set('filesize', intval(filesize($filename)));
                        break;

                    case "youtube-dl":
                        $objectv->set('filesize', intval(filesize($filenamepart)));
                        break;

                    case "playlist":
                        $objectv->set('filesize', 0);
                        break;

                    default:
                        $objectv->set('filesize', intval(filesize($filenamearia)));
                }
            } else {
                if ($objectv->get('dltype') == "playlist") {
                    $objectv->set('filesize', 0);
                } else {
                    if (file_exists($filename)) {
                        $objectv->set('filesize', intval(filesize($filename)));
                    }
                }
            }
            $objectsAssoc[] = $objectv->getAssoc();
        }
        // Filter the result.
        return $this->applyFilter($objectsAssoc, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function getDownload($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.common.objectuuid");
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        return $db->getAssoc("conf.service.downloader.download", $params['uuid']);
    }

    public function setDownload($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.downloader.setdownload");
        // Prepare the configuration object.
        $object = new \OMV\Config\ConfigObject("conf.service.downloader.download");
        $object->setAssoc($params);
        // Set the configuration object.
        $isNew = $object->isNew();
        $db = \OMV\Config\Database::getInstance();
        if (TRUE === $isNew) {
            // Check uniqueness - URL
            $db->assertIsUnique($object, "url");
        }
        $db->set($object);
        // Return the configuration object.
        return $object->getAssoc();
    }

    public function deleteDownload($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.common.objectuuid");
        // Destroy pid file
        $pid = sprintf("%sdownloader-%s.pid", $this->getPidDir, $params['uuid']);
        unlink($pid);
        // Delete the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.downloader.download", $params['uuid']);
        $db->delete($object);
        // Return the deleted configuration object.
        return $object->getAssoc();
    }

    public function doDownload($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.common.objectuuid");
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        // Get the configuration object.
        $object = $db->get("conf.service.downloader.download", $params['uuid']);

        $dltype    = $object->get('dltype');
        $filename  = $object->get('filename');
        $keepvideo = boolval($object->get('keepvideo'));
        $url       = $object->get('url');
        $delete    = boolval($object->get('delete'));

        if (!empty($object->get("sharedfolderref"))) {
            $sfObject = $db->get("conf.system.sharedfolder", $object->get("sharedfolderref"));
            // Get the mount point configuration object to build the absolute shared folder path.
            $meObject = $db->get("conf.system.filesystem.mountpoint", $sfObject->get("mntentref"));
            // Return the absolute shared folder path.
            $sfPath = build_path(DIRECTORY_SEPARATOR, $meObject->get("dir"), $sfObject->get("reldirpath"));
        }
        $ext = strtolower(substr($filename, -3));
        $kv  = "";
        switch ($dltype) {
            case "curl":
                $cmd = sprintf("%s -L -s -o \"%s/%s\" \"%s\" 2>&1 > /dev/null &", $this->getCurl, $sfPath, $filename, $url);
                break;
            case "youtube-dl":
                if ($keepvideo == true) {
                    $kv = "-k ";
                }
                if ($ext == "mp3" || $ext == "aac" || $ext == "wav" || $ext == "m4a" ) {
                    $filename = substr($filename, 0, strlen($filename) - 3) . "mp4";
                    $cmd = sprintf(
                        "%s -q -x %s--audio-format \"%s\" --newline --no-warnings -o \"%s/%s\" \"%s\" 2>&1",
                        $this->getYoutubedl, $kv, $ext, $sfPath, $filename, $url
                    );
                } else {
                    $cmd = sprintf("%s -q --newline --no-warnings -o \"%s/%s\" \"%s\" 2>&1 > /dev/null &",
                        $this->getYoutubedl, $sfPath, $filename, $url
                    );
                }
                break;
            case "playlist":
                $cmd = sprintf("cd %s && %s --newline --no-warnings --ignore-errors \"%s\"", $sfPath, $this->getYoutubedl, $url);
                file_put_contents("/tmp/output", $cmd);
                break;
            default:
                $cmd = sprintf("%s -q -d \"%s/\" -o \"%s\" \"%s\" 2>&1 > /dev/null &", $this->getAria2, $sfPath, $filename, $url);
        }
        // Create pid file
        $pid = sprintf("%sdownloader-%s.pid", $this->getPidDir, $params['uuid']);
        touch($pid);

        // [2013-10-30 13:30:15] DOWNLOAD: youtube-dl: test.mp4: movies: START: http://domain.com/test.mp4
        $log = sprintf("[%s] DOWNLOAD: %s: %s: %s: START: %s\r\n", date("Y-m-d H:i:s"), $dltype, $filename, $sfname, $url);
        file_put_contents($this->getLogFile, $log, FILE_APPEND);

        // Start download
        exec($cmd);

        $log = sprintf("[%s] DOWNLOAD: %s: %s: %s: FINISH: %s\r\n", date("Y-m-d H:i:s"), $dltype, $filename, $sfname, $url);
        file_put_contents($this->getLogFile, $log, FILE_APPEND);

        // Destroy pid file
        unlink($pid);

        if ( $delete ) {
            // Delete the configuration object.
            $db = \OMV\Config\Database::getInstance();
            $object = $db->get("conf.service.downloader.download", $params['uuid']);
            $db->delete($object);
        }
    }

    public function getUpload($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.downloader");
        // Remove useless properties from the object.
        $object->remove("downloads");
        return $object->getAssoc();
    }

    public function setUpload($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.downloader.setupload");
        // Get the existing configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.downloader");
        $object->setAssoc($params);
        $db->set($object);
        // Remove useless properties from the object.
        $object->remove("downloads");
        // Return the configuration object.
        return $object->getAssoc();
    }

    public function doUpload($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.downloader.doupload");
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.downloader");
        // Remove useless properties from the object.
        $object->remove("downloads");

        if (!empty($object->get("uploadref"))) {
            $sfObject = $db->get("conf.system.sharedfolder", $object->get("uploadref"));

            // Get the mount point configuration object to build the absolute shared folder path.
            $meObject = $db->get("conf.system.filesystem.mountpoint", $sfObject->get("mntentref"));

            // Return the absolute shared folder path.
            $sfPath = build_path(DIRECTORY_SEPARATOR, $meObject->get("dir"), $sfObject->get("reldirpath"));
            $uploadFile = sprintf("%s/%s", $sfPath, $params['filename']);
        }
        if ( file_exists($uploadFile) ) {
            throw new \OMV\Exception("File already exists.");
        }
        // Move file to shared folder selected
        if ( !rename( $params['filepath'], $uploadFile ) ) {
            throw new \OMV\Exception("Failed to move file");
        }
        // Change ownership of file
        if ( !chown( $uploadFile, $object->get('username') ) ) {
            throw new \OMV\Exception("Failed to change ownership of file");
        }
        if ( !chgrp( $uploadFile, "users" ) ) {
            throw new \OMV\Exception("Failed to change group of file");
        }
    }

    public function doUpdate($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();
        if($pid > 0) { // Parent process.
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }
        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus($bgStatusFilename, "outputfilename", $bgOutputFilename);

            // Execute command.
            $cmd = sprintf("%s -L# -o %s \"https://yt-dl.org/downloads/latest/youtube-dl\" 2>&1",
                $this->getCurl, $this->getYoutubedl
            );
            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);
            if ( !chmod( $this->getYoutubedl(), 0755 ) ) {
                throw new \OMV\Exception("Failed to make file executable");
            }
            if ( !chown( $this->getYoutubedl(), "root" ) ) {
                throw new \OMV\Exception("Failed to change ownership of file");
            }
            if ( !chgrp( $this->getYoutubedl(), "root" ) ) {
                throw new \OMV\Exception("Failed to change group of file");
            }
            exit(0);
        } catch(\Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }
}
