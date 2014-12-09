<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
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
require_once("openmediavault/object.inc");
require_once("openmediavault/config.inc");
require_once("openmediavault/error.inc");
require_once("openmediavault/util.inc");
require_once("openmediavault/rpcservice.inc");
require_once("openmediavault/notify.inc");

class OMVRpcServiceDownloader extends OMVRpcServiceAbstract
{
    const DOWNLOAD = 'download';
    const SYMLINK = 'symlink';

    public function getName()
    {
        return "Downloader";
    }

    private function getPluginName()
    {
        return strtolower( $this->getName() );
    }

    private function getXPath()
    {
        return sprintf( "//services/%s", $this->getPluginName() );
    }

    private function getXPathDownloads()
    {
        return sprintf( "%s/%ss", $this->getXPath(), self::DOWNLOAD );
    }

    private function getXPathDownload()
    {
        return sprintf( "%s/%s", $this->getXPathDownloads(), self::DOWNLOAD );
    }

    private function getXPathSymlinks()
    {
        return sprintf( "%s/%ss", $this->getXPath(), self::SYMLINK );
    }

    private function getXPathSymlink()
    {
        return sprintf( "%s/%s", $this->getXPathSymlinks(), self::SYMLINK );
    }

    private function getXPathSharedFolder()
    {
        return "//system/shares/sharedfolder";
    }

    private function getXPathMntEnt()
    {
        return "//system/fstab/mntent";
    }

    private function getEventMessagePath()
    {
        return sprintf( "org.openmediavault.services.%s", $this->getPluginName() );
    }

    private function getEventMessagePathDownload()
    {
        return sprintf( "%s.%ss.%s", $this->getEventMessagePath(), self::DOWNLOAD, self::DOWNLOAD );
    }

    private function getEventMessagePathSymlink()
    {
        return sprintf( "%s.%ss.%s", $this->getEventMessagePath(), self::SYMLINK, self::SYMLINK );
    }

    private function getYoutubedl()
    {
        return "/usr/local/bin/youtube-dl";
    }

    private function getAria2()
    {
        return "/usr/bin/aria2c";
    }

    private function getCurl()
    {
        return "/usr/bin/curl";
    }

    private function getLogFile()
    {
        return "/var/log/downloader.log";
    }

    private function getPidDir()
    {
        return "/var/run/";
    }

    public function initialize()
    {
        $this->registerMethod("getDownloads");
        $this->registerMethod("getDownload");
        $this->registerMethod("setDownload");
        $this->registerMethod("deleteDownload");
        $this->registerMethod("doDownload");

        $this->registerMethod("getUpload");
        $this->registerMethod("setUpload");
        $this->registerMethod("doUpload");
        $this->registerMethod("doUpdate");

        $this->registerMethod("getSymlinks");
        $this->registerMethod("getSymlink");
        $this->registerMethod("setSymlink");
        $this->registerMethod("deleteSymlink");
        $this->registerMethod("getRootFolders");
    }

    public function getDownloads($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "start"     : { "type" : "integer" },
                "limit"     : { '.$GLOBALS['OMV_JSONSCHEMA_COUNTFIELD'].' },
                "sortfield" : { '.$GLOBALS['OMV_JSONSCHEMA_SORTFIELD'].' },
                "sortdir"   : { '.$GLOBALS['OMV_JSONSCHEMA_SORTDIR'].' }
            }
        }');

        // Get configuration data.
        $objects = $xmlConfig->getList( $this->getXPathDownload() );
        if (is_null($objects))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPathDownload()
            );
        }

        // Add additional share informations.
        foreach($objects as $objectk => &$objectv)
        {
            $objectv['downloading'] = false;
            $objectv['filesize']    = "0";

            $objectv['sharedfoldername'] = gettext("n/a");
            // Get the shared folder configuration object.
            $sfObject = $xmlConfig->get("//system/shares/sharedfolder[uuid='{$objectv['sharedfolderref']}']");

            if (!is_null($sfObject) && !empty($sfObject))
                $objectv['sharedfoldername'] = $sfObject['name'];

            $sfUuid = $objectv["sharedfolderref"];
            if(empty($sfUuid))
                continue;

            // Get the shared folder path.
            $sfPath = OMVRpc::exec("ShareMgmt", "getPath", array("uuid" => $sfUuid), $context);
            $filename = sprintf("%s/%s", $sfPath, $objectv["filename"]);
            $filenamepart = sprintf("%s/%s.part", $sfPath, $objectv["filename"]);
            $filenamearia = sprintf("%s/%s.aria2", $sfPath, $objectv["filename"]);

            $pid = sprintf("%sdownloader-%s.pid", $this->getPidDir(), $objectv['uuid']);

            if (file_exists($pid))
            {
                $objectv['downloading'] = true;

                switch ($objectv['dltype'])
                {
                    case "curl":
                        $objectv['filesize'] = filesize($filename);
                        break;

                    case "youtube-dl":
                        $objectv['filesize'] = filesize($filenamepart);
                        break;

                    default:
                        $objectv['filesize'] = filesize($filenamearia);
                }
            }
            else
            {
                if (file_exists($filename))
                    $objectv['filesize'] = filesize($filename);
            }
        }

        // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function getDownload($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array("role" => OMV_ROLE_ADMINISTRATOR)
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

          // Get the configuration object.
        $xPath = sprintf("%s[uuid='%s']", $this->getXPathDownload(), $params['uuid']);
        $object = $xmlConfig->get($xPath);
        if (is_null($object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath
            );
        }

        // Modify the result object.
        $object['keepvideo'] = boolval($object['keepvideo']);

        return $object;
    }

    public function setDownload($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid"            : { '.$GLOBALS['OMV_JSONSCHEMA_UUID_UNDEFINED'].' },
                "dltype"          : { "type" : "string" },
                "filename"        : { "type" : "string", "optional" : true },
                "keepvideo"       : { "type" : "boolean", "optional" : true },
                "url"             : { "type" : "string" },
                "sharedfolderref" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' },
                "delete"          : { "type" : "boolean" }
            }
        }');

        // Prepare configuration data.
        $object = array(
            "uuid"            => ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ? OMVUtil::uuid() : $params['uuid'],
            "dltype"          => $params['dltype'],
            "filename"        => $params['filename'],
            "keepvideo"       => array_boolval($params, 'keepvideo'),
            "url"             => $params['url'],
            "sharedfolderref" => $params['sharedfolderref'],
            "delete"          => array_boolval($params, 'delete')
        );

        // Set the configuration data.
        $success = FALSE;
        if ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED'])
        {
            // Append object to configuration.
            $success = $xmlConfig->set($this->getXPathDownloads(), array("download" => $object));
        }
        else
        {
            // Update existing configuration object.
            $xPath = sprintf("%s[uuid='%s']", $this->getXPathDownload(), $params['uuid'] );
            $success = $xmlConfig->replace($xPath, $object);
        }
        if (FALSE === $success)
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED
            );
        }

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ? OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
            $this->getEventMessagePathDownload(),
            $object
        );

        // Return the configuration object.
        return $object;
    }

    public function deleteDownload($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Destroy pid file
        $pid = sprintf("%sdownloader-%s.pid", $this->getPidDir(), $params['uuid']);
        unlink($pid);

        // Delete the configuration object.
        $xPath = sprintf("%s[uuid='%s']", $this->getXPathDownload(), $params['uuid']);

        return $this->deleteConfigObjectByPath(
            $xPath,
            $this->getEventMessagePathDownload()
        );
    }

    public function doDownload($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type":"object",
            "properties":{
                "uuid":{'.$GLOBALS['OMV_JSONSCHEMA_UUID'].'}
            }
        }');

        // Get the configuration object.
        $xPath = sprintf("%s[uuid='%s']", $this->getXPathDownload(), $params['uuid']);
        $object = $xmlConfig->get($xPath);
        if (is_null($object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath
            );
        }

        $dltype    = $object['dltype'];
        $filename  = $object['filename'];
        $keepvideo = boolval($object['keepvideo']);
        $sfref     = $object['sharedfolderref'];
        $url       = $object['url'];
        $delete    = boolval($object['delete']);

        $xPath = sprintf("%s[uuid='%s']", $this->getXPathSharedFolder(), $sfref);
        $objectsf = $xmlConfig->get($xPath);
        if (is_null($objectsf))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath
            );
        }
        $mntentref = $objectsf['mntentref'];
        $path      = $objectsf['reldirpath'];
        $sfname    = $objectsf['name'];

        $xPath = sprintf("%s[uuid='%s']", $this->getXPathMntEnt(), $mntentref);
        $objectmt = $xmlConfig->get($xPath);
        if (is_null($objectsf))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath
            );
        }
        $dir = $objectmt['dir'];
        $ext = strtolower(substr($filename, -3));

        $kv = "";
        if( $keepvideo == true )
            $kv = "-k";

        switch ($dltype)
        {
            case "curl":
                $cmd = sprintf("%s -s -o \"%s/%s/%s\" \"%s\" 2>&1", $this->getCurl(), $dir, $path, $filename, $url );
                break;

            case "youtube-dl":
                if ($ext == "mp3" || $ext == "aac" || $ext == "wav" || $ext == "m4a" )
                {
                    $filename = substr($filename, 0, strlen($filename) - 3) . "mp4";
                    $cmd = sprintf("%s -q -x %s --audio-format \"%s\" --newline -o \"%s/%s/%s\" \"%s\" 2>&1", $this->getYoutubedl(), $kv, $ext, $dir, $path, $filename, $url );
                }
                else
                {
                    $cmd = sprintf("%s -q --newline -o \"%s/%s/%s\" \"%s\" 2>&1", $this->getYoutubedl(), $dir, $path, $filename, $url );
                }
                break;

            default:
                $cmd = sprintf("%s -q -d \"%s/%s/\" -o \"%s\" \"%s\" 2>&1", $this->getAria2(), $dir, $path, $filename, $url);
        }

        // Create pid file
        $pid = sprintf("%sdownloader-%s.pid", $this->getPidDir(), $params['uuid']);
        touch($pid);

        // [2013-10-30 13:30:15] DOWNLOAD: youtube-dl: test.mp4: movies: START: http://domain.com/test.mp4
        $log = sprintf("[%s] DOWNLOAD: %s: %s: %s: START: %s\r\n", date("Y-m-d H:i:s"), $dltype, $filename, $sfname, $url);
        file_put_contents($this->getLogFile(), $log, FILE_APPEND);

        if (0 !== $this->exec($cmd, $output))
        {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_FAILED,
                $cmd,
                implode("\n", $output)
            );
        }

        $log = sprintf("[%s] DOWNLOAD: %s: %s: %s: FINISH: %s\r\n", date("Y-m-d H:i:s"), $dltype, $filename, $sfname, $url);
        file_put_contents($this->getLogFile(), $log, FILE_APPEND);

        // Destroy pid file
        unlink($pid);

        if ( $delete )
        {
            // Delete the configuration object.
            $xPath = sprintf("%s[uuid='%s']", $this->getXPathDownload(), $params['uuid']);
            $this->deleteConfigObjectByPath(
                $xPath,
                $this->getEventMessagePathDownload()
            );
        }
    }

    public function getUpload($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array("role" => OMV_ROLE_ADMINISTRATOR)
        );

        // Get the configuration object.
        $object = $xmlConfig->get( $this->getXPath() );
        if (is_null($object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        // Remove useless fields from the result object.
        unset($object['downloads']);
        unset($object['symlinks']);

        return $object;
    }

    public function setUpload($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "username"  : { "type" : "string" },
                "uploadref" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Get the existing configuration object.
        $oldObject = $xmlConfig->get( $this->getXPath() );
        if (is_null($oldObject))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        // Prepare configuration data.
        $object = array(
            "username"  => $params['username'],
            "uploadref" => $params['uploadref'],
            "downloads" => isset($oldObject['downloads']) ? $oldObject['downloads'] : array(),
            "symlinks"  => isset($oldObject['symlinks']) ? $oldObject['symlinks'] : array()
        );

        if (FALSE === $xmlConfig->replace($this->getXPath(), $object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED
            );
        }

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            OMV_NOTIFY_MODIFY,
            $this->getEventMessagePath(),
            $object
        );

        // Return the configuration object.
        return $object;
    }

    public function doUpload($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "filename" : { "type" : "string" },
                "filepath" : { "type" : "string" }
            }
        }');

        // Get the configuration object.
        $object = $xmlConfig->get( $this->getXPath() );
        if (is_null($object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        unset($object['downloads']);
        unset($object['symlinks']);

        // Get the absolute shared folder path.
        $sfPath = OMVRpc::exec("ShareMgmt", "getPath", array("uuid" => $object['uploadref']), $context);
        $uploadFile = sprintf("%s/%s", $sfPath, $params['filename']);

        if ( file_exists($uploadFile) )
        {
            throw new OMVException(
                OMVErrorMsg::E_MISC_FAILURE,
                "File already exists."
            );
        }

        // Move file to shared folder selected
        if ( !rename( $params['filepath'], $uploadFile ) )
        {
            throw new OMVException(
                OMVErrorMsg::E_MISC_FAILURE,
                sprintf(
                    "Failed to move file '%s' to shared folder.",
                    $params['filename']
                )
            );
        }
        // Change ownership of file
        if ( !chown( $uploadFile, $object['username'] ) )
        {
            throw new OMVException(
                OMVErrorMsg::E_MISC_FAILURE,
                "Failed to change ownership of file"
            );
        }
        if ( !chgrp( $uploadFile, "users" ) )
        {
            throw new OMVException(
                OMVErrorMsg::E_MISC_FAILURE,
                "Failed to change group of file"
            );
        }
    }

    public function doUpdate($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();
        if ($pid > 0) { // Parent process.
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus($bgStatusFilename, "outputfilename", $bgOutputFilename);

            $cmd = sprintf("%s -L# -o %s \"https://yt-dl.org/downloads/latest/youtube-dl\" 2>&1", $this->getCurl(), $this->getYoutubedl() );
            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);

            if ( !chmod( $this->getYoutubedl(), 0755 ) )
            {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_FAILURE,
                    "Failed to make file executable"
                );
            }
            if ( !chown( $this->getYoutubedl(), "root" ) )
            {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_FAILURE,
                    "Failed to change ownership of file"
                );
            }
            if ( !chgrp( $this->getYoutubedl(), "root" ) )
            {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_FAILURE,
                    "Failed to change group of file"
                );
            }
            exit(0);
        } catch(Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }

    public function getSymlinks($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "start"     : { "type" : "integer" },
                "limit"     : { '.$GLOBALS['OMV_JSONSCHEMA_COUNTFIELD'].' },
                "sortfield" : { '.$GLOBALS['OMV_JSONSCHEMA_SORTFIELD'].' },
                "sortdir"   : { '.$GLOBALS['OMV_JSONSCHEMA_SORTDIR'].' }
            }
        }');

        // Get configuration data.
        $objects = $xmlConfig->getList( $this->getXPathSymlink() );
        if (is_null($objects))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPathSymlink()
            );
        }

        // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function getSymlink($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array("role" => OMV_ROLE_ADMINISTRATOR)
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

          // Get the configuration object.
        $xPath = sprintf("%s[uuid='%s']", $this->getXPathSymlink(), $params['uuid']);
        $object = $xmlConfig->get($xPath);
        if (is_null($object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath
            );
        }

        // Modify the result object.
        $object['enable'] = boolval($object['enable']);

        return $object;
    }

    public function setSymlink($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid"        : { '.$GLOBALS['OMV_JSONSCHEMA_UUID_UNDEFINED'].' },
                "enable"      : { "type" : "boolean" },
                "source"      : { "type" : "string" },
                "destination" : { "type" : "string" }
            }
        }');

        // Prepare configuration data.
        $object = array(
            "uuid"        => ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ? OMVUtil::uuid() : $params['uuid'],
            "enable"      => array_boolval($params, 'enable'),
            "source"      => $params['source'],
            "destination" => $params['destination']
        );

        // Set the configuration data.
        $success = FALSE;
        if ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED'])
        {
            $xPath = sprintf("%s[destination='%s']", $this->getXPathSymlink(), $params['destination']);
            if (TRUE === $xmlConfig->exists($xPath))
            {
                throw new OMVException(
                    OMVErrorMsg::E_CONFIG_OBJECT_UNIQUENESS,
                    gettext("This destination is already in use.")
                );
            }
            // Append object to configuration.
            $success = $xmlConfig->set($this->getXPathSymlinks(), array("symlink" => $object));
        }
        else
        {
            // Get the old configuration object.
            $xPath = sprintf("%s[uuid='%s']", $this->getXPathSymlink(), $params['uuid']);
            $oldObject = $xmlConfig->get($xPath);
            if (is_null($oldObject))
            {
                throw new OMVException(
                    OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                    $xPath
                );
            }

            // remove old symbolic link
            if (is_link($oldObject['destination']))
            {
                unlink($oldObject['destination']);
            }

            // Update existing configuration object.
            $xPath = sprintf("%s[uuid='%s']", $this->getXPathSymlink(), $params['uuid'] );
            $success = $xmlConfig->replace($xPath, $object);
        }
        if (FALSE === $success)
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED
            );
        }

        if ($object['enable'] === true)
        {
            // Create symbolic link
            $success = FALSE;
            $success = symlink($object['source'], $object['destination']);
            if (FALSE === $success)
            {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_FAILURE,
                    sprintf(
                        "Failed to create symbolic link at %s",
                        $object['destination']
                    )
                );
            }
        }

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ? OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
            $this->getEventMessagePathSymlink(),
            $object
        );

        // Return the configuration object.
        return $object;
    }

    public function deleteSymlink($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Get the configuration object.
        $xPath = sprintf("%s[uuid='%s']", $this->getXPathSymlink(), $params['uuid']);
        $object = $xmlConfig->get($xPath);
        if (is_null($object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath
            );
        }

        // remove old symbolic link
        if (is_link($object['destination']))
        {
            unlink($object['destination']);
        }

        // Delete the configuration object.
        $xPath = sprintf("%s[uuid='%s']", $this->getXPathSymlink(), $params['uuid']);

        return $this->deleteConfigObjectByPath(
            $xPath,
            $this->getEventMessagePathSymlink()
        );
    }

    public function getRootFolders($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // List the directory contents.
        $result = array();
        $path = sprintf("/%s", rtrim($params['path'], "/"));
        if ( strlen($path) == 0 )
            $path = "/";

        foreach (new DirectoryIterator($path) as $item)
        {
            if ($item->isDot())
                continue;
            if (!$item->isDir())
                continue;
            $result[] = $item->getFilename();
        }
        return $result;
    }

}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceDownloader());
