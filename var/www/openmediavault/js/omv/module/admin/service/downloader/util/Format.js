/**
 *
 * @license http://www.gnu.org/licenses/gpl.html GPL Version 3
 * Copyright (C) 2011-2012 Marcel Beck <marcel.beck@mbeck.org>
 * Copyright (C) 2013-2014 OpenMediaVault Plugin Developers
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

// require("js/omv/util/Format.js")

Ext.ns("OMV.module.services.downloader.util");

OMV.module.services.downloader.util.Format = function() {
    var f = function() {};
    var o = function() {};
    f.prototype = OMV.util.Format;

    Ext.extend(o, f, function() {
        return {
            bytesToSize : function(bytes) {
                var sizes = ['Bytes', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB', 'EiB', 'ZiB', 'YiB'];

                if (bytes === 0)
                    return 'n/a';

                var i = parseInt(Math.floor(Math.log(bytes) / Math.log(1024)), 10);

                return ((i === 0) ? (bytes / Math.pow(1024, i)) : (bytes / Math.pow(1024, i)).toFixed(1)) + ' ' + sizes[i];
            },

            timeInterval : function(seconds) {
                var weeks = Math.floor(seconds / 604800), days = Math.floor((seconds % 604800) / 86400), hours = Math.floor((seconds % 86400) / 3600), minutes = Math.floor((seconds % 3600) / 60), secondsLeft = Math.floor(seconds % 60), w = weeks + 'w', d = days + 'd', h = hours + 'h', m = minutes + 'm', s = secondsLeft + 's';

                if (weeks)
                    return w + ' ' + d;
                if (days)
                    return d + ' ' + h;
                if (hours)
                    return h + ' ' + m;
                if (minutes)
                    return m + ' ' + s;

                return s;
            },

            rate : function(Bps) {
                var speed = Math.floor(Bps / 1000);

                if (speed <= 999.95)// 0 KBps to 999 K
                    return [speed.toTruncFixed(0), 'KB/s'].join(' ');

                speed /= 1000;

                if (speed <= 99.995)// 1 M to 99.99 M
                    return [speed.toTruncFixed(2), 'MB/s'].join(' ');
                if (speed <= 999.95)// 100 M to 999.9 M
                    return [speed.toTruncFixed(1), 'MB/s'].join(' ');

                // Insane speeds
                speed /= 1000;
                return [speed.toTruncFixed(2), 'GB/s'].join(' ');
            },

            fsRenderer : function (value, metadata, record) {
                var fs = parseInt(value);
                return OMV.module.services.downloader.util.Format.bytesToSize(fs);
            }
        };
    }());

    return new o();
}();

Number.prototype.toTruncFixed = function(place) {
    var ret = Math.floor(this * Math.pow(10, place)) / Math.pow(10, place);
    return ret.toFixed(place);
};

Number.prototype.toStringWithCommas = function() {
    return this.toString().replace(/\B(?=(?:\d{3})+(?!\d))/g, ",");
};
