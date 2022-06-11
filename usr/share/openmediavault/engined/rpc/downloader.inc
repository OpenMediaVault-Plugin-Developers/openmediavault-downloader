<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2021 OpenMediaVault Plugin Developers
 * @copyright Copyright (c) 2022 Godofallthatwas
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
    private $getAria2 = "/usr/bin/aria2c";
    private $getCurl = "/usr/bin/curl";
    private $getLogFile = "/var/log/downloader.log";
    private $getPidDir = "/var/run/";
    private $getYoutubedl = "/usr/bin/youtube-dl";

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
        $this->registerMethod("doDownloadBg");

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
            $finalExt = $objectv->get('format');
            $finalfilename = sprintf("%s/%s.%s", $sfPath, $objectv->get("filename"), $finalExt);
            $tmpExt = "tmp";
            $filenamepart = sprintf("%s/%s.%s.part", $sfPath, $objectv->get("filename"), $tmpExt);
            $filenamearia = sprintf("%s/%s.aria2", $sfPath, $objectv->get("filename"));
            $pid = sprintf("%sdownloader-%s.pid", $this->getPidDir, $objectv->get('uuid'));
            $dlType = $objectv->get('dltype');

            if (file_exists($pid)) {
                $objectv->set('downloading', true);

            }
            if ($dlType == "playlist" || $dlType == "multiple") {
                $objectv->set('filesize', 0);
            } else {
                if (file_exists($filename)) {
                    //curl 
                    $objectv->set('filesize', intval(filesize($filename)));
                } elseif (file_exists($finalfilename)) {
                    //Youtube-dl, Playlist, multiple Done
                    $objectv->set('filesize', intval(filesize($finalfilename)));
                } elseif (file_exists($filenamearia)) {
                    //Aria
                    $objectv->set('filesize', intval(filesize($filenamearia)));
                } elseif (file_exists($filenamepart)) {
                    //Youtube-dl in progress
                    $objectv->set('filesize', intval(filesize($filenamepart)));
                }
            }
            
            $objectv->set('url', str_replace(PHP_EOL, '<br/>', $objectv->get('url')));
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

        if (!empty($object->get("sharedfolderref"))) {
            $sfObject = $db->get("conf.system.sharedfolder", $object->get("sharedfolderref"));
            // Get the mount point configuration object to build the absolute shared folder path.
            $meObject = $db->get("conf.system.filesystem.mountpoint", $sfObject->get("mntentref"));
            // Return the absolute shared folder path.
            $sfPath = build_path(DIRECTORY_SEPARATOR, $meObject->get("dir"), $sfObject->get("reldirpath"));
        }

        $dltype = $object->get('dltype');
        $filename = $object->get('filename');
        $url = $object->get('url');
        $delete = boolval($object->get('delete'));
        $finalExt = $object->get('format');
        $tmpExt = "tmp";

# I think we need a best audio, and a best video (technically 'best' since a large point of the option would be best file that's a straight download)
# -x is specifically for audio, 
#  bestvideo+bestaudio on sites that offer it (youtube) with ffmpeg will basically force the file to be a perticular format with --merge-output-format
# When we hard code the extension, and youtube-dl falls back to the last "best" it's basically going to give the file the wrong ext (with merge output I think the only time we run into this might be wma files?.
#  We can have youtube-dl set the extension correctly, but I'd like the user to be infored somehow that their "webm" has been renamed "mp4" because that's all that was available.
# I honestly don't know what's actually better bestXXXXX[ext=xxx]/bestXXXX  or just bestxxxx
# I had a situation where setting it to mp4 maxed out at 720p because that's the best mp4 youtube had on file
# but set it to webm and it jumped up to 2160p because that's the best webm they had,
# setting bestvideo+bestaudio instead of bestvideo[ext=mp4]+bestaudio[ext=mp4] and set --merge-output-format getst me a 2160 mp4
# but it just dumps the VP8 + opus streams into a mp4 container, rather then converting video to h.264... 
# --reencode-video will fix that, but (as one should expect) it's a lot slower

        switch ($object->get('format')) {
            case "aac":
                $format = "bestaudio[ext=aac]/bestaudio";
                $audio = true;
                break;
            case "m4a":
                $format = "bestaudio[ext=m4a]/bestaudio";
                $audio = true;
                break;
            case "mp3":
                $format = "bestaudio[ext=mp3]/bestaudio";
                $audio = true;
                break;
            case "mp4";
                $format = "bestvideo[ext=mp4]+bestaudio[ext=mp4]/best[ext=mp4]/bestvideo+bestaudio/best";
                break;
            case "wav":
                $format = "bestaudio[ext=wav]/bestaudio";
                $audio = true;
                break;
            case "webm";
                 $format = "bestvideo[ext=webm]+bestaudio[ext=webm]/best[ext=webm]/bestvideo+bestaudio/best";
                break;
        }
        $cmdArgs = [];

        switch ($dltype) {
            case "curl":
                $cmdBin = $this->getCurl;
                $cmdArgs[] = "--location";
                $cmdArgs[] = "--silent";
                $cmdArgs[] = sprintf("--output \"%s/%s\"", $sfPath, $filename);
                $cmdArgs[] = sprintf("\"%s\"", $url);
                break;

            case "youtube-dl":
                $cmdBin = $this->getYoutubedl;
                $cmdArgs[] = "--quiet";
                $cmdArgs[] = "--newline";
                $cmdArgs[] = "--no-warnings";
                $cmdArgs[] = "--no-playlist";
                if ($object->get('subtitles')) {
                    $cmdArgs[] = "--all-subs --write-sub";
                }
                if ($audio){
                    //Audio: Sometimes ends up turncated without using a tmpExt.
                    $cmdArgs[] = sprintf("-x --audio-format '%s' --format '%s'", $finalExt, $format);
                    $cmdArgs[] = sprintf("-o \"%s/%s.%s\"", $sfPath, $filename, $tmpExt);
                }
                else{
                    //Video: Seems Ok jumping straight to finalExt
                    $cmdArgs[] = sprintf("--format '%s' --merge-output-format '%s'", $format, $finalExt);
                    $cmdArgs[] = sprintf("-o \"%s/%s.%s\"", $sfPath, $filename, $finalExt);
                }
                $cmdArgs[] = sprintf("--no-part --no-exec \"%s\"", $url);
                break;

            case "playlist":
                $cmdBin = sprintf("cd %s && %s", $sfPath, $this->getYoutubedl);
                $cmdArgs[] = "--newline";
                $cmdArgs[] = "--no-warnings";
                $cmdArgs[] = "--ignore-errors";
                if ($object->get('subtitles')) {
                    $cmdArgs[] = "--all-subs --write-sub";
                }
                if ($audio){
                    $cmdArgs[] = sprintf("-x --audio-format '%s' --format '%s'", $finalExt, $format);
                }
                else{
                    $cmdArgs[] = sprintf("--format '%s' --merge-output-format '%s'", $format, $finalExt);
                }
                $cmdArgs[] = sprintf("\"%s\"", $url);
                break;

            case "multiple":
#Our youtubedl can actually make a decent run at a random (say pdf) file but only(?) if you leave format unconfigured, or 'best'... Adding multiple formats, causes it to realize it's not(?) a media file and fail. Not sure this is inteneded behaviour, so not sure we should act on it.
#aren't playlist and Multiple esentially the same on the backend, is there any reason not to merge them, playlist doesn't make a tmp file that seems to be the only difference.  
                $tmpfname = tempnam("/tmp", "multiple");
                file_put_contents($tmpfname, $url);

                $cmdBin = sprintf("cd %s && %s", $sfPath, $this->getYoutubedl);
                $cmdArgs[] = "--newline";
                $cmdArgs[] = "--no-warnings";
                $cmdArgs[] = "--ignore-errors";
                if ($object->get('subtitles')) {
                    $cmdArgs[] = "--all-subs --write-sub";
                }
                if ($audio){
                    $cmdArgs[] = sprintf("-x --audio-format '%s' --format '%s'", $finalExt, $format);
                }
                else{
                    $cmdArgs[] = sprintf("--format '%s' --merge-output-format '%s'", $format, $finalExt);
                }
                $cmdArgs[] = sprintf("--batch-file \"%s\"", $tmpfname);
                break;

            default:
                $cmdBin = $this->getAria2;
                $cmdArgs[] = "--quiet=true";
                $cmdArgs[] = sprintf("--dir \"%s/\"", $sfPath);
                $cmdArgs[] = sprintf("--out \"%s\"", $filename);
                $cmdArgs[] = sprintf("\"%s\"", $url);
        }

        // Create pid file
        $pid = sprintf("%sdownloader-%s.pid", $this->getPidDir, $params['uuid']);
        touch($pid);

        // [2013-10-30 13:30:15] DOWNLOAD: youtube-dl: test.mp4: movies: START: http://domain.com/test.mp4
        $log = sprintf("[%s] DOWNLOAD: %s: %s: %s: START: %s\r\n", date("Y-m-d H:i:s"), $dltype, $filename, $sfPath, $url);
        file_put_contents($this->getLogFile, $log, FILE_APPEND);

        // Start download
        $cmd = new \OMV\System\Process($cmdBin, $cmdArgs);
        $cmd->setQuiet(TRUE);
        $cmd->execute();

        $log = sprintf("[%s] DOWNLOAD: %s: %s: %s: FINISH: %s\r\n", date("Y-m-d H:i:s"), $dltype, $filename, $sfPath, $url);
        file_put_contents($this->getLogFile, $log, FILE_APPEND);

        // Destroy pid file
        unlink($pid);

        if ( $delete ) {
            // Delete the configuration object.
            $db = \OMV\Config\Database::getInstance();
            $object = $db->get("conf.service.downloader.download", $params['uuid']);
            $db->delete($object);
        }

        // Destroy url list file
        if (!empty($tmpfname) && file_exists($tmpfname)) {
            unlink($tmpfname);
        }
    }

    public function doDownloadBg($params, $context) {
        return $this->callMethodBg("doDownload", $params, $context);
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
        // Create the background process.
        return $this->execBgProc(function($bgStatusFilename, $bgOutputFilename)
            use ($params) {
                $cmd = new \OMV\System\Process("omv-updateytdl");
                $cmd->setRedirect2to1();
                $cmdLine = $cmd->getCommandLine();
                if (0 !== $this->exec($cmdLine, $output, $bgOutputFilename)) {
                    throw new \OMV\ExecException($cmdLine, $output);
                }
                return $output;
            }
        );
    }
}
