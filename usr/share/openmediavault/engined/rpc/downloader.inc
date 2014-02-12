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

class OMVRpcServiceDownloader extends OMVRpcServiceAbstract {
    /**
     * Get the RPC service name.
     */
    public function getName() {
        return "Downloader";
    }

    /**
     * Initialize the RPC service.
     */
    public function initialize() {
        $this->registerMethod("getDownloads");
        $this->registerMethod("getDownload");
        $this->registerMethod("setDownload");
        $this->registerMethod("deleteDownload");
        $this->registerMethod("doDownload");
        $this->registerMethod("doSilent");
        $this->registerMethod("getUpload");
        $this->registerMethod("setUpload");
        $this->registerMethod("doUpload");
    }

    /**
     * Get downloads
     */
    public function getDownloads($params, $context) {
        global $xmlConfig;
        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

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
        $xpath = "//services/downloader/download";
        $objects = $xmlConfig->getList($xpath);
        if(is_null($objects)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);
        }

        // Add additional share informations.
        foreach($objects as $objectk => &$objectv) {
            $objectv['sharedfoldername'] = gettext("n/a");
            // Get the shared folder configuration object.
            $sfObject = $xmlConfig->get("//system/shares/sharedfolder[uuid='{$objectv['sharedfolderref']}']");

            if(!is_null($sfObject) && !empty($sfObject)) {
                $objectv['sharedfoldername'] = $sfObject['name'];
            }
        }

        // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    /**
     * Get a download configuration object.
     */
    public function getDownload($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array("role" => OMV_ROLE_ADMINISTRATOR));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

          // Get the configuration object.
        $xpath = sprintf("//services/downloader/download[uuid='%s']", $params['uuid']);
        $object = $xmlConfig->get($xpath);
        if(is_null($object)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);
        }

        return $object;
    }

    /**
     * Set (add/update) a download.
     */
    public function setDownload($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid"            : { '.$GLOBALS['OMV_JSONSCHEMA_UUID_UNDEFINED'].' },
                "dltype"          : { "type" : "string" },
                "filename"        : { "type" : "string", "optional" : true },
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
            "url"             => $params['url'],
            "sharedfolderref" => $params['sharedfolderref'],
            "delete"          => array_boolval($params, 'delete')
        );

        // Set the configuration data.
        $success = FALSE;
        if($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) {
            // Append object to configuration.
            $success = $xmlConfig->set("//services/downloader", array("download" => $object));
        } else {
            // Update existing configuration object.
            $success = $xmlConfig->replace("//services/downloader/download[uuid='{$params['uuid']}']", $object);
        }
        if(FALSE === $success) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED);
        }

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ?
            OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
            "org.openmediavault.services.downloader.download", $object);

        // Return the configuration object.
        return $object;
    }

    /**
     * Delete a link configuration object.
     */
    public function deleteDownload($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Delete the configuration object.
        $xpath = sprintf("//services/downloader/download[uuid='%s']", $params['uuid']);

        return $this->deleteConfigObjectByPath($xpath,
            "org.openmediavault.services.downloader.download");
    }

    function doDownload($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type":"object",
            "properties":{
                "uuid":{'.$GLOBALS['OMV_JSONSCHEMA_UUID'].'}
            }
        }');

        // Get the configuration object.
        $xpath = sprintf("//services/downloader/download[uuid='%s']", $params['uuid']);
        $object = $xmlConfig->get($xpath);
        if(is_null($object)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);
        }

        $dltype = $object['dltype'];
        $filename = $object['filename'];
        $sfref = $object['sharedfolderref'];
        $url = $object['url'];
        $delete = boolval($object['delete']);

        $xpath = sprintf("//system/shares/sharedfolder[uuid='%s']", $sfref);
        $objectsf = $xmlConfig->get($xpath);
        if(is_null($objectsf)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);
        }

        $mntentref = $objectsf['mntentref'];
        $path = $objectsf['reldirpath'];

        $xpath = sprintf("//system/fstab/mntent[uuid='%s']", $mntentref);
        $objectmt = $xmlConfig->get($xpath);
        if(is_null($objectsf)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);
        }

        $dir = $objectmt['dir'];

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

            switch ($dltype)
            {
                case "curl":
                    $cmd = "/usr/bin/curl -o \"$dir/$path/$filename\" \"$url\" 2>&1";
                    break;

                case "youtube-dl":
                    $cmd = "/usr/local/bin/youtube-dl --newline -o \"$dir/$path/$filename\" \"$url\" 2>&1";
                    break;

                default:
                    if ( strlen($filename) > 0 )
                        $cmd = "/usr/bin/aria2c -d \"$dir/$path/\" -o \"$filename\" \"$url\" 2>&1";
                    else
                        $cmd = "/usr/bin/aria2c -d \"$dir/$path/\" \"$url\" 2>&1";

            }

            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);

            if ( $delete ) {
                // Delete the configuration object.
                $xpath = sprintf("//services/downloader/download[uuid='%s']", $params['uuid']);

                $this->deleteConfigObjectByPath($xpath, "org.openmediavault.services.downloader.download");
            }
            exit(0);
        } catch(Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }

    function doSilent($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type":"object",
            "properties":{
                "uuid":{'.$GLOBALS['OMV_JSONSCHEMA_UUID'].'}
            }
        }');

        // Get the configuration object.
        $xpath = sprintf("//services/downloader/download[uuid='%s']", $params['uuid']);
        $object = $xmlConfig->get($xpath);
        if(is_null($object)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);
        }

        $dltype = $object['dltype'];
        $filename = $object['filename'];
        $sfref = $object['sharedfolderref'];
        $url = $object['url'];
        $delete = boolval($object['delete']);

        $xpath = sprintf("//system/shares/sharedfolder[uuid='%s']", $sfref);
        $objectsf = $xmlConfig->get($xpath);
        if(is_null($objectsf)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);
        }

        $mntentref = $objectsf['mntentref'];
        $path = $objectsf['reldirpath'];

        $xpath = sprintf("//system/fstab/mntent[uuid='%s']", $mntentref);
        $objectmt = $xmlConfig->get($xpath);
        if(is_null($objectsf)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);
        }

        $dir = $objectmt['dir'];

        switch ($dltype)
        {
            case "curl":
                $cmd = "/usr/bin/curl -s -o \"$dir/$path/$filename\" \"$url\" 2>&1";
                break;

            case "youtube-dl":
                $cmd = "/usr/local/bin/youtube-dl -q --newline -o \"$dir/$path/$filename\" \"$url\" 2>&1";
                break;

            default:
                if ( strlen($filename) > 0 )
                    $cmd = "/usr/bin/aria2c -q -d \"$dir/$path/\" -o \"$filename\" \"$url\" 2>&1";
                else
                    $cmd = "/usr/bin/aria2c -q -d \"$dir/$path/\" \"$url\" 2>&1";
        }

        if(0 !== $this->exec($cmd, $output)) {
            throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));
        }

        if ( $delete ) {
            // Delete the configuration object.
            $xpath = sprintf("//services/downloader/download[uuid='%s']", $params['uuid']);
            $this->deleteConfigObjectByPath($xpath, "org.openmediavault.services.downloader.download");
        }
    }

    /**
     * Get download settings
     */
    public function getUpload($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array("role" => OMV_ROLE_ADMINISTRATOR));

        // Get the configuration object.
        $xpath = "//services/downloader";
        $object = $xmlConfig->get($xpath);
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        // Remove useless fields from the result object.
        unset($object['download']);
        return $object;
    }

    /**
     * Set upload settings
     */
    public function setUpload($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "username"  : { "type" : "string" },
                "uploadref" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Prepare configuration data.
        $object = array(
            "username"  => $params['username'],
            "uploadref" => $params['uploadref'],
            "download"  => isset($oldObject['download']) ? $oldObject['download'] : array()
        );

        if(FALSE === $xmlConfig->replace("//services/downloader", $object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED);

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_MODIFY,
            "org.openmediavault.services.downloader", $object);

        // Return the configuration object.
        return $object;
    }

    function doUpload($params, $context) {
        // Validate the RPC caller context.
        $this->validateMethodContext( $context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "filename" : { "type" : "string" },
                "filepath" : { "type" : "string" }
            }
        }');

        // Get the configuration object.
        $xpath = "//services/downloader";
        $object = $xmlConfig->get($xpath);
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        unset($object['download']);

        // Get the absolute shared folder path.
        $sfPath = OMVRpc::exec("ShareMgmt", "getPath", array("uuid" => $object['uploadref']), $context);
        $uploadFile = sprintf("%s/%s", $sfPath, $params['filename']);

        if ( file_exists($uploadFile) )
            throw new OMVException(OMVErrorMsg::E_MISC_FAILURE, "File already exists.");

        // Move file to shared folder selected
        if( !rename( $params['filepath'], $uploadFile ) ) {
            throw new OMVException(OMVErrorMsg::E_MISC_FAILURE, sprintf(
                "Failed to move file '%s' to shared folder.", $params['filename']));
        }
        // Make script executable
        if( !chown( $uploadFile, $object['username'] ) )
            throw new OMVException(OMVErrorMsg::E_MISC_FAILURE, "Failed to change ownership of file");
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceDownloader());
?>
