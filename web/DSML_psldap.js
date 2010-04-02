/*
 * mod_psldap
 *
 * User Authentication against and maintenance of an LDAP database
 *
 * Copyright (C) 2004 David Picard dpicard@psind.com
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
var isNav=0, isMoz=0, isIE=0, isOpera=0;

if (parseInt(navigator.appVersion) >= 4) {
    if (navigator.appName == "Netscape" ) {
        isNav = true;
    } else if (navigator.appName == "Mozilla") {
	isMoz = true;
    } else if (navigator.appName == "Opera") {
	isOpera = true;
    } else {
        isIE = true;
    }
}

// Update the URI with the bound URIs in the httpd.conf
//var psldapRootUri = document.URL.substring(0, document.URL.lastIndexOf("/"));
var psldapSitePrefix = document.URL.substring(8);
psldapSitePrefix = "http://" + psldapSitePrefix.substring(0,psldapSitePrefix.indexOf("/"));
var psldapRootUri = "/psldap";
var ldapupdateUri = "/ldapupdate";

var currentRecord = 0;
var formElmts = null;
var recordNbrElmt = null;

/** Alert with xml content **/
function showMyXML() {
    if (window.document.XMLDocument.xml) {
        alert(window.document.XMLDocument.xml);
    } else {
        alert(window.document.textContent);
    }
}

/** Provides the value associate with name from loaded cookies
 *  @param c_name the cookie name
 *  @return the unescaped value stored in the cookie
 **/
function ps_get_cookie ( c_name )
{
    var c = null;
    if (document.cookie) {
	index = document.cookie.indexOf(c_name);
	if (index != -1) {
	    var v_start = (document.cookie.indexOf("=", index) + 1);
	    var v_end = document.cookie.indexOf(";", v_start);
	    if (v_end == -1) {
		v_end = v_start;
	    }
	    c = document.cookie.substring(v_start, v_end);
	}
	/*
    } else {
	window.alert("Cookies not accessible, personalization disabled");
	*/
    }
    return c;
}

function getLoginUserCN() {
    return ps_get_cookie("psUserCn");
}

function getLoginUserDN() {
    return ps_get_cookie("psUserDn");
}

/** @return true if images can be represented inline in base64 encoding as
 *           opposed to referencing the src as a URI
 **/
function uaSupportsInlineImages() {
    var result = (isNav || isMoz);
    return result;
}

function writeDomainOptions() {
    for (var i=0; i < ldapDomains.length; i++) {
	document.write('<option ' + ((ldapDomains[i].defaultDomain==1)?"selected ":"") + 'value="' + ldapDomains[i].dn + '">' + ldapDomains[i].label + '</option>');
    }
}

/** Forces the visibility of the images in the objSpan to be shown or hidden
 *  @param objSpan the span whose images are to be altered
 *  @param bShow true indicates the images should be shown, false indicates a
 *         hidden state is desired.
 **/
function showNodeManagementImages(objSpan, bShow) {
    var j;
    if ((objSpan.nodeType != 3) && (objSpan.tagName != "SPAN")) {
        objSpan = getMySpan(objSpan);
    }
    for (j = 0; j < objSpan.childNodes.length; j++) {
        var cNode = objSpan.childNodes[j];
        if ((cNode.nodeType != 3) && /* not a text node */
            (cNode.tagName == "IMG") && (cNode.name != "delete") ) {
            cNode.style.display = (bShow) ? "inline" : "none";
        }
    }
}

/** Internal function - changes style display settings to show/hide elements
 *  @param ss - style sheet reference
 *  @param j - position of rule to change
 *  @param rule - rule reference to change
 *  @param dispStyle - string containing block, none, or inline
 **/
function changeDisplayStyle(ss, j, rule, dispStyle)
{
    var selectorText = rule.selectorText;
    if (undefined != ss.removeRule) { ss.removeRule(j); }
    else { ss.deleteRule(j); }
    if (undefined != ss.addRule) {
	ss.addRule(selectorText, "display: " + dispStyle + ";", j);
    } else {
	ss.insertRule(selectorText + " { display: " + dispStyle + "; }", j);
    }
}

/** Resets the personal, work, other, and im styles to be visible as indicated
 *  @param personal - use 'none' to hide or 'block' to show
 *  @param work - use 'none' to hide or 'block' to show
 *  @param other - use 'none' to hide or 'block' to show
 *  @param im - use 'none' to hide or 'block' to show
 **/
function showInfo(personal, work, other, im, vendor )
{
    var i = document.styleSheets.length;
    while (i-- > 0) {
	var ss = document.styleSheets[i];
	var rules = ss.cssRules;
	var j;
	if (undefined == rules) rules = ss.rules;
	j = rules.length;

	while ( j-- > 0) {
	    var rule = rules[j];
	    if (undefined == ss.cssRules) {
		rule = rules.item(j);
	    }
	    switch(rule.selectorText) {
	    case "DIV.personal_info":
	    case "div.personal_info":
		changeDisplayStyle(ss, j, rule, personal);
		break;
	    case "DIV.work_info":
	    case "div.work_info":
		changeDisplayStyle(ss, j, rule, work);
		break;
	    case "DIV.other_info":
	    case "div.other_info":
		changeDisplayStyle(ss, j, rule, other);
		break;
	    case "DIV.im_info":
	    case "div.im_info":
		changeDisplayStyle(ss, j, rule, im);
		break;
	    case "DIV.vendor_info":
	    case "div.vendor_info":
		changeDisplayStyle(ss, j, rule, vendor);
		break;
	    }
	}
    }
}

/** Clear the input element value within the specified span. Currently only
 *  implemented for the "text" type.
 *  @param objSpan the span whose input elements are to have their values
 *         cleared.
 **/
function clearSpanInputValues(objSpan) {
    var i;
    for (i = 0; i < objSpan.childNodes.length; i++) {
        var cNode = objSpan.childNodes[i];
        if ((cNode.nodeType != 3) && /* not a text node */
            (cNode.tagName == "INPUT") ) {
            cNode.value = "";
        }
    }
}

/** Increments the name sequence by the specified amount.
 *  @param objSpan the span whose input elements are to be incremented.
 *  @param inc the numeric value by which to increment
 **/
function incrementSpanInputNames(objSpan, inc) {
    var i;
    for (i = 0; i < objSpan.childNodes.length; i++) {
        if ((objSpan.childNodes[i].nodeType != 3) && /* not a text node */
            (objSpan.childNodes[i].tagName == "INPUT") ) {
            var j = objSpan.childNodes[i].name.split("-");
            if ((j) && (j.length > 1)) {
                objSpan.childNodes[i].name = j[0] + "-" + (Number(j[1]) + inc);
            }
        }
    }
}

/** Clone the span containing the element el, hiding the element and
 *  appending the new span after the current span. Also ensures the ID
 *  is increment such that any ending patterns of "-#" are replaced
 *  with an incremented number. This is only intended to work on the
 *  last span in the data set.
 *  @param el any HTMLElement within the span to be cloned
 **/
function cloneAndAppendCurrentSpan(el) {
    var mySpan = getMySpan(el);
    if (null != mySpan) {
        var newSpan = mySpan.cloneNode(true);
        incrementSpanInputNames(newSpan, 1);
        clearSpanInputValues(newSpan);
        mySpan.parentNode.appendChild(newSpan);
        showNodeManagementImages(mySpan, false);
        newSpan.onchange="showNodeManagementImages(this,this.value!='')";
    }
}

/** Deleted the current span, removing it from the document. Also
 *  ensures the names of subsequent spans are altered such that any
 *  ending patterns of "-#" are contiguous and are replaced with
 *  an decremented number.
 *  @param el any HTMLElement within the span to be deleted
 **/
function deleteCurrentSpan(el) {
    var mySpan = getMySpan(el);
    if (null != mySpan) {
        var pNode = mySpan.parentNode;
        var nextSpan = mySpan.nextSibling;
        var prevSpan = mySpan.previousSibling;
        
        if (null != nextSpan) {
            /* Renumber name attributes of subsequent childNodes */
            while (null != nextSpan) {
                incrementSpanInputNames(nextSpan, -1);
                nextSpan = nextSpan.nextSibling;
            }
            pNode.removeChild(mySpan);
        } else if (null != prevSpan) {
            /* Deleting last node - show controls on prev node */
            showNodeManagementImages(prevSpan, true);
            prevSpan.onchange="showNodeManagementImages(this,this.value!='')";
            pNode.removeChild(mySpan);
        } else {
            /* Only child - don't delete node - clear value in mySpan */
            clearSpanInputValues(mySpan);
        }
    }
}

/** Gets all form elements in the specified document. If no document
 *  is specified it defaults to the current document.
 *  @param objWindowArg (optional) a reference to a Window object
 *  @return an array containing all elements found. An empty array if
 *          no elements are found.
 **/
function getAllFormElements(objWindowArg) {
    var objWindow = (arguments.length < 1) ? window : objWindowArg;
    var result = formElmts;
    if ((null == formElmts) || (arguments.length > 0)) {
        result = objWindow.document.getElementsByTagName("form");
        if (objWindow == window) {
            formElmts = result;
        }
    } else {
        result = formElmts;
    }
    return result;
}

function resetProcessWindow() {
    var wt = document.getElementById("processWindow");
    if (null != wt) {
        wt.src = wt.src;
    }
}

function showProcessDocument(bForceShow) {
    var processDivObj = document.getElementById("processDiv");
    var bForce = false;
    if (arguments.length > 0) bForce = bForceShow; 

    if (null == processDivObj) {
        processDivObj = parent.document.getElementById("processDiv");
    }
    if (null != processDivObj) {
        //var objXml = processDivObj.processWindow.contentWindow.document.getElementsByTagName("XML");
        //if (bForce || (0 < objXml.length) ) {
	if (bForce) {
	    processDivObj.style.display = "block";
	    processDivObj.style.zIndex = 99;
        } else {
	    processDivObj.style.display = "none";
	    processDivObj.style.zIndex = 0;
        }
    }
}

/**
 **/
function psldapCopyAttributes(dest, src)
{
    var i;
    for (i = src.attributes.length - 1; i >= 0; i--) {
        var objAttr = src.attributes.item(i);
        dest.setAttribute(objAttr.name, objAttr.value);
    }
    dest.className = src.className;
}

/** Implementation of the document importNode function to fix IE's broken 
 *  implementation of the DOM 2 spec. This function must be applied to a
 *  document object.
 *  @param objNode the node to be imported into the current document
 *  @param bDeep (optional) boolean value to indicate if the node is to be
 *               copied recursively default value is false.
 *  @return the newly imported node.
 **/
function psldapImportNode(objNode, bDeep)
{
    var i;
    var result = this.createElement(objNode.tagName);
    psldapCopyAttributes(result, objNode);
/*
    for (i = objNode.attributes.length - 1; i >= 0; i--) {
        var objAttr = objNode.attributes.item(i);
        result.setAttribute(objAttr.name, objAttr.value);
    }
*/
    if (bDeep) {
/*
        for (i = 0; i < objNode.childNodes.length; i++) {
            var objChild = this.importNode(objNode.childNodes[i], bDeep);
            result.appendChild(objChild);
        }
*/
        result.innerHTML = objNode.innerHTML;
    }
    return result;
}

function getBaseDNFromMyUrl() {
    var result = document.URL.indexOf("dn=");
    if (-1 != result) {
        result = unescape(document.URL.substring(result+3));
    } else {
        result = "";
    }
    return result;
}

/** Clones form to target window
 *  @param objForm the Form element to clone
 *  @param wt the target window in which to clone the form
 *  @return cloned node element if clonable - else original form reference
 **/
function cloneFormToWindow(objForm, wt) {
    var objClone = objForm;
    if ((wt.document.importNode) &&
	(objForm.encoding != "multipart/form-data") ) {
	/* IE does not support import node - the work arounds seem to
	   be deficient when used with forms, so just submit the form
	   in the context of the current page. Also multipart forms
	   tend to have file input elements which cannot be cloned. */
	/*wt.document.importNode = psldapImportNode;*/
	var objBody = wt.document.getElementsByTagName("BODY")[0];
	objClone = wt.document.importNode(objForm, true);
	objClone.target = "_self";
	/* Force rendering */
	objClone.style.display = "block";
	wt.document.body.replaceChild(objClone, objBody.childNodes[0]);
	/* Copy over all the entered data */
	for (i = 0; i < objForm.elements.length; i++) {
	    if ((objForm.elements[i].value) ||
		((objForm.elements[i].name) &&
		 (objClone.elements[objForm.elements[i].name].value)) ) {
		objClone.elements[objForm.elements[i].name].value =
		    objForm.elements[i].value;
	    } else if ((objForm.elements[i].checked) ||
		       ((objForm.elements[i].name) &&
			(objClone.elements[objForm.elements[i].name].checked) ) ) {
		objClone.elements[objForm.elements[i].name].checked =
		    objForm.elements[i].checked;
	    } else if ((objForm.elements[i].selectedIndex) ||
		       ((objForm.elements[i].name) &&
			(objClone.elements[objForm.elements[i].name].selectedIndex)) ) {
		objClone.elements[objForm.elements[i].name].selectedIndex =
		    objForm.elements[i].selectedIndex;
	    } else {
		window.status = "Not copying value " + i + ":" + objForm.elements[i].name;
	    }
	}
    }
    return objClone;
}

/** Sets the value of the distinguished name in the form based on the form
 *  data.
 *  @param objForm the form containing the data and the hidden dn input elmt
 **/
function setDNFromFormData(objForm) {
    var objOrgElement = (objForm.elements["o"]) ? objForm.elements["o"] :
	objForm.elements["o-1"];
    var objOUElement = (objForm.elements["ou"]) ? objForm.elements["ou"] :
	objForm.elements["ou-1"];
    var objCNElement = (objForm.elements["cn"]) ? objForm.elements["cn"] :
	objForm.elements["cn-1"];
    
    objForm.dn.value = getBaseDNFromMyUrl();
    if ("" == objForm.dn.value) {
	objForm.dn.value = prompt("Enter the base DN to contain this record");
    }
    
    if (objOrgElement && (objOrgElement.value != "")) {
	objForm.dn.value = "o=" + objOrgElement.value + ", " + objForm.dn.value;
    }
    
    if (objOUElement && (objOUElement.value != "")) {
	objForm.dn.value = "ou=" + objOUElement.value + ", " + objForm.dn.value;
    }
    
    if (objCNElement) {
	if (objForm.elements["givenName"] && objForm.elements["sn"] ) {
	    objCNElement.value = objForm.elements["givenName"].value + " " +
		objForm.elements["sn"].value;
	}
	else if (objForm.elements["givenName-1"] && objForm.elements["sn-1"] ) {
	    objCNElement.value = objForm.elements["givenName-1"].value + " " +
		objForm.elements["sn-1"].value;
	}
	if (objCNElement.value != "") {
	    objForm.dn.value = "cn=" + objCNElement.value + ", " + objForm.dn.value;
	}
    }
    objForm.dn.value = window.prompt("Confirm the DN for this record:", objForm.dn.value);
}

function moveVisibleRecord(action) {
    var objForm = getAllFormElements();
    objForm = objForm[currentRecord-1];

    if (objForm.elements["newrdn"]) {
	objForm.newrdn.value = prompt("Enter the new RDN for this record", objForm.newrdn.value);
    }
    if (objForm.elements["newSuperior"]) {
	objForm.newSuperior.value = prompt("Enter the new superior for this record", objForm.newSuperior.value);
    }
    if (objForm.elements["newrdn"] && objForm.elements["newSuperior"]) {
	submitVisibleRecord(action);
    } else {
	alert("New RDN and new Superior are not defined - aborting move request");
    }
}

/** Copies the visible form into an iframe labeled as "processWindow" in the
 *  current document and submits the cloned item, capturing the result in the
 *  iframe and maintaining the integrity of the pending form.
 *  @param action one of three string constants: Create, Delete, Update
 **/
function submitVisibleRecord(action) {
    var objForm = getAllFormElements();
    var wt = document.getElementById("processWindow");
    var i;
    if (null != wt) {
	/* reset the content to the cleanSrc page to ensure submission ability */
        wt = wt.contentWindow;
    }
    
    objForm = objForm[currentRecord-1];
    objForm.FormAction.value = action;
    if (action == "Create") {
	setDNFromFormData(objForm);
    }
    
    if (wt.confirm("Are you sure you wish to " + action + " this record?") ) {
	//var objClone = cloneFormToWindow(objForm, wt);
	var objClone = objForm;
        showProcessDocument(true);
        objClone.submit();
    }
}

function resetVisibleRecord() {
    var objForm = getAllFormElements();
    objForm[currentRecord-1].reset();
}

function toggleClassInfo() {
    var aForms = getAllFormElements();
    var myForm = null;
    var objClassDiv = null;
    if ((currentRecord > 0) && (currentRecord <= aForms.length)) {
        var i;
        myForm = aForms[currentRecord-1];
        for ( i = 0; i < myForm.elements.length; i++) {
            if (myForm.elements[i].name == "objectClass-1") {
                objClassDiv = myForm.elements[i];
                break;
            }
        }
    }
    if (null != objClassDiv) {
	while ((undefined == objClassDiv.tagName) ||
	       !((objClassDiv.tagName.toUpperCase() == "DIV") &&
		 (((undefined != objClassDiv.name) &&
		   (objClassDiv.name.toUpperCase() == "OCEDIT")) ||
		  ((undefined != objClassDiv.className) &&
		   (objClassDiv.className.toUpperCase() == "OCEDIT")) ) )
	       ) {
	    objClassDiv = objClassDiv.parentNode;
	}
	if (undefined == objClassDiv) {
	    alert("Could not find objectclass component");
	} else if (objClassDiv.style.display != "block") {
	    objClassDiv.style.display = "block";
	} else if (objClassDiv.style.display != "none") {
	    objClassDiv.style.display = "none";
	} else {
	    alert("Could not determine display style for classes: " + objClassDiv.style.display);
	}
    } else {
        alert("Class info modification not supported for this record");
    }
}

function getMyRow(objElement) {
    var myRow = objElement;
    while ((null != myRow) && (myRow.nodeType != 3) && /* not a text node */
           (myRow.tagName != "TR")) {
        myRow = myRow.parentNode;
    }
    return myRow;
}

function getMySpan(objElement) {
    var mySpan = objElement;
    while ((null != mySpan) && (mySpan.nodeType != 3) && /* not a text node */
           (mySpan.tagName != "SPAN")) {
        mySpan = mySpan.parentNode;
    }
    return mySpan;
}

function getMyDiv(objElement) {
    var myDiv = objElement;
    while ((null != myDiv) && (myDiv.nodeType != 3) && /* not a text node */
           (myDiv.tagName != "DIV")) {
        myDiv = myDiv.parentNode;
    }
    return myDiv;
}

function pvt_setRecordDisplay(aForms, position, displayStyle)
{
    var myDiv = null;
    if ((position > 0) && (position <= aForms.length)) {
        myDiv = getMyDiv(aForms[position-1]);
    }
    if (null != myDiv) {
        if (myDiv.style.display != displayStyle) {
            myDiv.style.display = displayStyle;
        }
    }
}

function showRecord(current) {
    resetProcessWindow();

    var objForm = getAllFormElements();
    if ((current > 0) && (current <= objForm.length)) {
        if (currentRecord >= 0) {
            pvt_setRecordDisplay(objForm, currentRecord, "none");
        }
        currentRecord = current;
        pvt_setRecordDisplay(objForm, currentRecord, "inline");
    }
    if (objForm.length > 1) {
        recordNbrElmt.value = currentRecord;
    }
}

function showNextRecord() {
    showRecord(currentRecord + 1);
}

function showPrevRecord() {
    showRecord(currentRecord - 1);
}

function loadRecordUrl(sel, target, olcb) {
    var objWindow = null;
    if (sel != "") {
	var objFrame = null;
        if ((arguments.length < 2) || (null == target)) {
            objWindow = window.open(sel, "", "resizable=yes, menubar=no, toolbar=no, height=480, width=544");
	    objFrame = objWindow;
        } else {
            objFrame = document.getElementById(target);
            objFrame.src = sel;
        }
	if ((null != objFrame) && (arguments.length > 2)) objFrame.onload=olcb;
    }
    return objWindow;
}

function setNodeHideAttrs(objElmt, bHide, bHideChildren, strNamePrefix) {
    var objChild;
    if (bHide) {
        if (objElmt.style) {
            objElmt.style.display = "none";
        }
        if (null != strNamePrefix) {
            if (objElmt.name && (undefined != objElmt.name) && 
                (0 != objElmt.name.indexOf(strNamePrefix))) {
                objElmt.name = strNamePrefix + objElmt.name;
            }
        }
    } else {
        if (objElmt.style) {
            objElmt.style.display = "inline";
        }
        if (null != strNamePrefix) {
            if (objElmt.name && (undefined != objElmt.name) && 
                (0 == objElmt.name.indexOf(strNamePrefix)) ) {
                objElmt.name = objElmt.name.substring(7);
            }
        }
    }
    if (bHideChildren && objElmt.firstChild &&
        (undefined != objElmt.firstChild) ) {
        var objChild;
        var nextChild;
        for (objChild = objElmt.firstChild; null != objChild;
             objChild = nextChild) {
            nextChild = objChild.nextSibling;
            setNodeHideAttrs(objChild, bHide, true, strNamePrefix);
        }
    }
}

function hideAndGetParentByTagName(myElmt, argBChangeName, strParentTagName) {
    var objElmt = myElmt;
    var bChangeName = ((arguments.length < 1) || argBChangeName);

    setNodeHideAttrs(objElmt, true, false,
                       ((bChangeName) ? "hidden_" : null) );
    if (arguments.length > 2) {
        while ((null != objElmt) &&
               (0 != objElmt.tagName.indexOf(strParentTagName)) ) {
            objElmt = objElmt.parentNode;
        }
        if (null == objElmt) objElmt = myElmt;
    }
    setNodeHideAttrs(objElmt, true, true,
                       ((bChangeName) ? "hidden_" : null) );
    return objElmt;
}

function showPreviousSiblingAndHide(myElmt, argBChangeName, strParentTagName) {
    var objElmt = myElmt;
    var mySibling = null;
    var bChangeName = ((arguments.length < 1) || argBChangeName);
    
    if (arguments.length > 2) {
        objElmt = hideAndGetParentByTagName(myElmt, bChangeName,
                                            strParentTagName);
    } else {
        objElmt = hideAndGetParentByTagName(myElmt, bChangeName);
    }
    objElmt = objElmt.previousSibling;
    setNodeHideAttrs(objElmt, false, true,
                       ((bChangeName) ? "hidden_" : null) );

}

function showNextSiblingAndHide(myElmt, argBChangeName, strParentTagName) {
    var objElmt = myElmt;
    var mySibling = null;
    var bChangeName = ((arguments.length < 1) || argBChangeName);
    
    if (arguments.length > 2) {
        objElmt = hideAndGetParentByTagName(myElmt, bChangeName,
                                            strParentTagName);
    } else {
        objElmt = hideAndGetParentByTagName(myElmt, bChangeName);
    }
    objElmt = objElmt.nextSibling;
    setNodeHideAttrs(objElmt, false, true,
                       ((bChangeName) ? "hidden_" : null) );

}

function getXmlNodeByDN(xmlDom) {
    var result = xmlDom;
    var candidates = xmlDom.getElementsByTagName("searchResultEntry");
    for (var i = 0; i < candidates.length; i++) {
        var c = candidates[i];
        if (0 == c.getAttribute("dn").indexOf(getXmlNodeByDN.dn)) {
            window.status = "Found XML element for " + getXmlNodeByDN.dn;
            result = c;
            i = candidates.length;
        }
    }
    return result;
}

/** This function write the editable record to the target frame and requires
    psajax functionality...
 **/
function writeEditableRecord(dn, target, xslName, xslUri, xslManager,
                             docTargetId) {
    var theMgr = (arguments.length > 3) ? xslManager : xslmgr;
    theMgr.addStylesheet(xslName, xslUri);
    getXmlNodeByDN.dn = dn;
    theMgr.transform(xslName, getXmlNodeByDN, target, docTargetId);
}

function getEditableRecord(dn, target, xmgr) {
    var bhref = uaSupportsInlineImages() ? "off" : "on";
    if (arguments.length < 3) {
        var getUrl = ldapupdateUri + "?FormAction=Search&" +
	    "search=(objectClass=*)&scope=base&dn=" + encodeURIComponent(dn) +
            "&BinaryHRef=" + bhref +
            "&xsl1=" + psldapRootUri + "/DSML_editform.xsl";
	//+ "&xsl2=" + psldapRootUri + "/DSML_cards.xsl";
        loadRecordUrl(getUrl, target);
    } else {
        window.status = "Targeting window / iframe " + top.document.getElementById(target).id;
        var targetDoc = top.document.getElementById(target);
        var targetNode = "editableRecords";
        if ((null != target) && (undefined != target.contentWindow)) {
            targetDoc = target.contentWindow.document;
        } else {
            targetDoc = top.document;
            targetNode = "editFrame";
        }
        writeEditableRecord(dn, targetDoc, "edit",
                            psldapRootUri + "/DSML_editform.xsl",
                            xmgr, targetNode);
    }
}

function loadTemplateRecord(xmlUri, xslUri, target, passArgs, olcb) {
    var addArgs = (arguments.length < 4) ? "" : passArgs;
    var bhref = uaSupportsInlineImages() ? "off" : "on";
    var getUrl = ldapupdateUri + "?FormAction=Present&" +
	"xmlObjectTemplate=" + encodeURIComponent(xmlUri) +
	"&BinaryHRef=" + bhref +
	"&xsl1=" + xslUri + addArgs;
    return (arguments.length < 5) ? loadRecordUrl(getUrl, target): loadRecordUrl(getUrl, target, olcb);
}

function parseCNFromDN(dn) {
    var result = null;
    var index = dn.indexOf("cn=");

    if (index != -1) {
	var v_start = index + 3;
	var v_end = dn.indexOf(",", v_start);
	if (v_end > v_start) {
	    result = dn.substring(v_start, v_end);
	}
    }
    return result;
}

function getVCard(dn, target) {
    var userCN = parseCNFromDN(dn);
    var getUrl;
    if (null == userCN) userCN = "Contact";
    getUrl = ldapupdateUri + "?FormAction=Search" +
	"&search=(objectClass=*)&scope=base&dn=" + encodeURIComponent(dn) +
	"&BinaryType=text/x-vcard" +
	"&dlFilename=" + userCN.replace(/ /g, "") + ".vcf" +
        "&BinaryHRef=off" +
        "&xsl1=" + psldapRootUri + "/DSML_vcard.xsl" +
        "&xsl2=" + psldapRootUri + "/DSML_cards.xsl";
    loadRecordUrl(getUrl, target);
}

function getWindowHeight(objWindow) {
    var myHeight = 0;

    if( objWindow.innerHeight && typeof( objWindow.innerHeight ) == 'number' ) {
        //Non-IE
        myHeight = objWindow.innerHeight;
    } else if( document.documentElement &&
	       ( undefined != document.documentElement.clientHeight ) &&
	       ( 0 < document.documentElement.clientHeight ) ) {
        //IE 6+ in 'standards compliant mode'
        myHeight = document.documentElement.clientHeight;
    } else if( document.body && ( undefined != document.body.clientHeight ) ) {
        //IE 4 / 5 compatible
        myHeight = document.body.clientHeight;
    }
    if (myHeight < 100) {
	window.alert ("Window height: " + objWindow.innerHeight + ", " + 
		      document.documentElement.clientHeight + ", " +
		      document.body.clientHeight);
	myHeight = 150;
    }
    return myHeight;
}

function getClientHeight(cellElmt)
{
    var ht = 0;
    for (var j = 0; j < cellElmt.childNodes.length; j++) {
	if (undefined != cellElmt.childNodes[j].clientHeight) {
	    ht = ht + cellElmt.childNodes[j].clientHeight;
	}
    }
    return ht;
}


function verticalWrapNColumns(objCellElmt, ncol)
{
    var resSz = 0;
    var wrapHeight = (objCellElmt.lastChild.offsetTop)/ncol;
    var winH = getWindowHeight(window);
    var currentElmt = objCellElmt;

    while (currentElmt.offsetParent) {
	resSz += currentElmt.offsetTop;
	currentElmt = currentElmt.offsetParent;
    }
    winH += resSz;
    
    if (wrapHeight > winH) {
	wrapHeight = winH - wrapHeight;
    } else {
	wrapHeight = winH;
    }
    verticalWrapChildren(objCellElmt, wrapHeight);
}

function verticalWrapChildren(objCellElmt, reservedSize)
{
    var nextChild;
    /* Take 25 off the window height for the scrollbar thickness ... */
    var scrollThickness = 25;
    var wrapHeight = 0;
    var currentElmt = objCellElmt;
    while (currentElmt.offsetParent) {
	reservedSize += currentElmt.offsetTop;
	currentElmt = currentElmt.offsetParent;
    }

    currentElmt = objCellElmt;
    do {
	if (0 == wrapHeight) {
	    wrapHeight = getWindowHeight(window) - scrollThickness - reservedSize;
	}
        if (getClientHeight(currentElmt) >= wrapHeight) {
            if ((currentElmt != objCellElmt) &&
		(currentElmt.firstChild != currentElmt.lastChild) ) {
                /* Shift the previously moved element back and remove any
                   trailing breaking space */
                nextChild = currentElmt.removeChild(currentElmt.lastChild);
            } else if (objCellElmt.firstChild != objCellElmt.lastChild) {
		nextChild = objCellElmt.removeChild(objCellElmt.firstChild);
	    }

	    if ((currentElmt != objCellElmt) &&
		(getClientHeight(objCellElmt) < wrapHeight) ) {
		currentElmt = objCellElmt;
	    } else {
		currentElmt = document.createElement(objCellElmt.tagName);
		currentElmt.name = "wrapElmt";
		currentElmt.className = objCellElmt.className;
		objCellElmt.parentNode.insertBefore(currentElmt, objCellElmt);
	    }
	} else if (currentElmt != objCellElmt) {
	    nextChild = objCellElmt.removeChild(objCellElmt.firstChild);
	} else {
	    nextChild = undefined;
	}
	
	if (undefined != nextChild) {
	    if (currentElmt != objCellElmt) {
		currentElmt.appendChild(nextChild);
	    } else {
		currentElmt.insertBefore(nextChild, currentElmt.firstChild);
		/* In this case, the last column has been reached and the height
		   exceeds the screen height ... stop creating columns */
		/* Append another column to split the last if more than one...
		if ( currentElmt.firstChild != currentElmt.lastChild) {
		    nextChild = currentElmt.lastChild;
		    currentElmt = document.createElement(objCellElmt.tagName);
		    currentElmt.name = "wrapElmt";
		    currentElmt.className = objCellElmt.className;
		    objCellElmt.parentNode.insertAfter(currentElmt, objCellElmt);
		    currentElmt.appendChild(nextChild);
		}
		*/
		nextChild = undefined;
	    }
	}
    } while (undefined != nextChild);
}

function initializeCardsCB() {
    var cardTable = document.getElementById("cardTable");
    var cardTd = cardTable;
    while ((null != cardTd) && (0 != cardTd.tagName.indexOf("TD"))) {
        cardTd = cardTd.firstChild;
    }
    if (null != cardTd) {
	if (window.reservedSizeValue < 0) {
	    verticalWrapNColumns(cardTd, -1 * window.reservedSizeValue);
	} else {
	    verticalWrapChildren(cardTd, window.reservedSizeValue);
	}
    }
    window.status = "Done";
}

function initializeCards(reservedSize) {
    window.status = "Organizing cards...";
    // Allow rendering to finish ... wrap after a timeout
    window.reservedSizeValue = reservedSize;
    window.setTimeout(initializeCardsCB, 20);
}

function initialize() {
    window.status = "Loading forms...";

    recordNbrElmt = document.getElementById("recordNumber");

    showRecord(1);
    window.status = getAllFormElements().length + " records loaded";
}

function sortByTreeParentId(objItem1, objItem2)
{
    if (!objItem1.parentElmtCount) {
        objItem1.parentElmtCount = objItem1.treeParentId.split(",").length;
    }
    if (!objItem2.parentElmtCount) {
        objItem2.parentElmtCount = objItem2.treeParentId.split(",").length;
    }
    var result = objItem1.parentElmtCount - objItem2.parentElmtCount;
    if (0 == result) {
        result = objItem1.treeParentId.localeCompare(objItem2.treeParentId);
    }
    if (0 == result) {
        result = objItem1.id.localeCompare(objItem2.id);
    }
    return result;
}

function sortByTreeParentIdManager(objItem1, objItem2)
{
    var result = 0;
    if (0 == result) {
        result = objItem1.treeParentId.localeCompare(objItem2.treeParentId);
    }
    if (0 == result) {
        result = objItem1.id.localeCompare(objItem2.id);
    }
    return result;
}

function buildOrgTree(tableIdStr, recordNameStr, rowIdStr, parentDelimStr)
{
    // TODO - incorporate parentDelimStr
    var objTable = document.getElementById(tableIdStr);

    if (null == objTable) {
        alert("Could not find table " + tableIdStr + " to build tree");
    } else {
        var objElmtArray;
        if (objTable.rows) {
            objElmtArray = objTable.rows;
        } else {
            objElmtArray = document.getElementsByName(recordNameStr);
        }
        var objRecord;
        var objRecordArray = new Array();
        var parentRegex = /([^,]*,)/;
        var managerTree = (0 == parentDelimStr.indexOf("manager"));

        for (var i = 0; i < objElmtArray.length; i++) {
            var manager = null;
            var reNormal = /\s*,\s*/g;
            objRecord = objElmtArray[i];
            if (managerTree) {
                objRecord.treeParentId = objRecord.getAttribute("manager");
            } else  {
                objRecord.treeParentId = objRecord.getAttribute(rowIdStr).replace(parentRegex, "");
            }

            // Assign parent and normalize white space in id attributes
            objRecord.treeParentId = objRecord.treeParentId.replace(reNormal, ", ");
            objRecord.setAttribute(rowIdStr,
                   objRecord.getAttribute(rowIdStr).replace(reNormal, ", "));
            objRecord.id = objRecord.id.replace(reNormal, ", ");

            objRecordArray.push(objRecord);
        }

        if (managerTree) {
            objRecordArray.sort(sortByTreeParentIdManager);
        }  else {
            objRecordArray.sort(sortByTreeParentId);

            // Iterate in reverse, identify each child group, removing it from
            //   the array and moving those tr nodes under a table within the
            //   tr node of the parent record.
            while (--i >= 0) {
                var j = i;
                var grandparentLength = objRecordArray[i].parentElmtCount - 1;
                var objParentRecord = null;
                var childGroup = new Array(objRecordArray[i]);
                while ((--j >= 0) &&
                       (0 == objRecordArray[j].treeParentId.localeCompare(
                                           objRecordArray[i].treeParentId))
                      ) {
                    childGroup.push(objRecordArray[j]);
                }
                i = j + 1;
    
                // Find the parent record
                while ((j >= 0) && (null == objParentRecord) &&
                       (objRecordArray[j].parentElmtCount >= grandparentLength)) {
                    if ((objRecordArray[j].parentElmtCount == grandparentLength) &&
                        (0 == objRecordArray[j].getAttribute(rowIdStr).localeCompare(
                                               childGroup[0].treeParentId) ) ) {
                        objParentRecord = objRecordArray[j];
                    }
                    j--;
                }
    
                // Add nodes in childGroup as DOM children of a table element
                //   under the parent record. If there is no parent, leave them
                //   at the top level
                if (null != objParentRecord) {
                    var objTableElmt = document.createElement("TABLE");
                    var objTableBody = document.createElement("TBODY");
                    var objParentCell = objParentRecord;
                    objTableElmt.appendChild(objTableBody);
                    objTableBody.style.width = "100%";
                    objTableElmt.style.width = "100%";
    
                    while ((null != objParentCell) &&
                           (0 != objParentCell.tagName.indexOf("TD"))) {
                        objParentCell = objParentCell.firstChild;
                    }
    
                    objParentCell = objParentCell.nextSibling;
                    objParentCell.appendChild(document.createElement("BR"));
                    objParentCell.appendChild(objTableElmt);
                    while (0 < childGroup.length) {
                        var childRecord = childGroup.pop();
                        objTableBody.appendChild(childRecord);
    
                        var strNewLabel = childRecord.getAttribute(rowIdStr).replace(/([^=]*)=([^,]*),.*/,"$2");
                        var newTextNode = document.createTextNode(strNewLabel);
                        var objAnchor = childRecord.firstChild.nextSibling.firstChild;
                        //if (objAnchor.tagName != "A") {
                        //    objAnchor = objAnchor.nextSibling;
                        //}
                        while (objAnchor.hasChildNodes()) {
                            objAnchor.removeChild(objAnchor.firstChild);
                        }
                        objAnchor.appendChild(newTextNode);
                    }
                }
            }
        }
        addNavigationToTree(tableIdStr);
    }
}
