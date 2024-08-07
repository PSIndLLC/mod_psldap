/*
 * mod_psldap
 *
 * User Authentication against and maintenance of an LDAP database
 *
 * Copyright (C) 2024 PSInd, LLC
 *
 * http://www.psind.com/projects/mod_psldap/
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of version 2 of the GNU General Public License as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
var isNav = false, isIE = false;

if (parseInt(navigator.appVersion) >= 4) {
    if (navigator.appName == "Netscape" ) {
        isNav = true;
    } else {
        isIE = true;
    }
}

