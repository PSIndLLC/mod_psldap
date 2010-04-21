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
var psldapBaseUri = "";
var psldapRootUri = "/psldap";
var ldapupdateUri = "/ldapupdate";

function DSML_psldap_init() {
    try {
	psldapBaseUri = ps_siteConfig.secureBase;
	psldapRootUri = ps_siteConfig.homeURI;
	ldapupdateUri = ps_siteConfig.updateURI;
	recordNbrElmt = document.getElementById("recordNumber");
    } catch(e1) {
	window.status = "Failed initializing with psldap config...";
    }
}

/* A race condition may exist in loading the JS files - force reinitialization
   after the document is loaded */
try {
    DSML_psldap_init();
} catch(e1) {
    if (window.addEventListener) {
	window.addEventListener("load", DSML_psldap_init, true);
    } else if (window.attachEvent) {
	window.attachEvent("onload", DSML_psldap_init, true);
    } else {
	window.onload = DSML_psldap_init;
    }
}

function psdnd_deferred_init() {
    attachDnDEventsForElementTypeWithDNAttr(document, "img");
}

if (window.addEventListener) {
    window.addEventListener("load", psdnd_deferred_init, true);
} else if (window.attachEvent) {
    window.attachEvent("onload", psdnd_deferred_init, true);
} else {
    window.onload = psdnd_deferred_init;
}

var currentRecord = 0;
var formElmts = null;
var recordNbrElmt = null;

var ps_dndState = { srcDn: "", targetDn: "", srcOc: "", targetOc: "", srcCtxt: "", targetCtxt: "", getSrcName: new Function("return this.srcDn.replace(/([^=]*)=([^,]*),.*/,'$2')"), getTargetName: new Function("return this.targetDn.replace(/([^=]*)=([^,]*),.*/,'$2')"), dndForm: null };

var ps_dndActionBindings = [
       { srcOc: "groupOfUniqueNames", targetOc: "OpenLDAPperson", action: ps_dndChangeOwner, invert: true },
       { srcOc: "groupOfUniqueNames", targetOc: "inetOrgPerson", action: ps_dndChangeOwner, invert: true },
       { srcOc: "groupOfUniqueNames", targetOc: "person", action: ps_dndChangeOwner, invert: true },
       { srcOc: "inetOrgPerson",      targetOc: "inetOrgPerson", action: ps_dndChangeManager, invert: false },
       { srcOc: "OpenLDAPou",         targetOc: "groupOfUniqueNames", action: null, invert: false },
       { srcOc: "organizationalUnit", targetOc: "groupOfUniqueNames", action: null, invert: false },
       { srcOc: "organization",       targetOc: "", action: null, invert: false },
       { srcOc: "OpenLDAPorg",        targetOc: "", action: null, invert: false },
       { srcOc: "",                   targetOc: "organization", action: ps_dndModifyDN, invert: false },
       { srcOc: "",                   targetOc: "OpenLDAPorg", action: ps_dndModifyDN, invert: false },
       { srcOc: "",                   targetOc: "OpenLDAPou", action: ps_dndModifyDN, invert: false },
       { srcOc: "",                   targetOc: "organizationalUnit", action: ps_dndModifyDN, invert: false },
       { srcOc: "",                   targetOc: "groupOfUniqueNames", action: ps_dndAddMember, invert: true }
			    ];


function ps_findOrCreateNextLevel(el)
{
    var result = null;

    el = el.lastChild;
    if (0 != el.lastChild.tagName.indexOf("TABLE")) {
	var objTableElmt = document.createElement("table");
	var objTableBody = document.createElement("tbody");

	objTableBody.style.width = "100%";
	objTableElmt.style.width = "100%";
	objTableElmt.appendChild(objTableBody);
	el.appendChild(document.createElement("BR"));
	el.appendChild(objTableElmt);
	addNavigationToTree(objTableElmt);
	
	result = objTableBody;
    } else {
	result = el.lastChild.lastChild;
    }

    return result;
}

function ps_getLDAPRecordRow(dn)
{
    var result = null;
    var elmts = document.getElementsByName("LDAPRecord");
    var i;
    var dnStr = dn.replace(/\s*,[ ]*\s*/g, ", ");
    for(i = 0; i < elmts.length; i++) {
	if(0 == elmts[i].getAttribute('id').replace(/\s*,[ ]*\s*/g, ", ").localeCompare(dnStr)) {
	    result = elmts[i];
	    break;
	}
    }
    return result;
}

function ps_moveRowToNode(dn, newMgrDn)
{
    var srcElmt = ps_getLDAPRecordRow(dn);
    var targetElmt = ps_getLDAPRecordRow(newMgrDn);

    if (srcElmt && targetElmt) {
	var childTbl = ps_findOrCreateNextLevel(targetElmt);
	srcElmt.parentNode.removeChild(srcElmt);
	childTbl.appendChild(srcElmt);
    } else {
	window.status = srcElmt +":" + targetElmt;
    }
    return srcElmt;
}

function ps_dndChangeManager(invert, mode) {
    this.dndForm.FormAction.value = "Modify";

    if (arguments.length > 1) {
	var src = (invert) ? this.targetDn : this.srcDn;
	var target = (invert) ? this.srcDn : this.targetDn;
	var srcNm = (invert) ? this.getTargetName() : this.getSrcName();
	var targetNm = (invert) ? this.getSrcName() : this.getTargetName();
	if (mode == "message") {
	    return window.confirm("Change manager of " + srcNm + " to " +
				  targetNm );
	}
	else if ((mode == "fixTree" ) && (this.srcCtxt  == "mgmtTree") &&
		 (this.targetCtxt  == "mgmtTree") ) {
	    window.status = "Fixing management tree";
	    if (null != ps_moveRowToNode(src, target)) {
		if (ps_dndState.xslmgr) { ps_dndState.xslmgr.setXmXML(); }
	    } else {
		location.reload(true);
	    }
	} else {
	    alert(this.srcCtxt + ":" + this.targetCtxt);
	}
    } else if (invert) {
	this.dndForm.dn.value = this.targetDn;
	this.dndForm['manager-1'].value = this.srcDn;
    } else {
	this.dndForm.dn.value = this.srcDn;
	this.dndForm['manager-1'].value = this.targetDn;
    }
}

function ps_dndAddMember(invert, mode) {
    this.dndForm.FormAction.value = "AddAttributes";

    if (arguments.length > 1) {
	var src = (invert) ? this.targetDn : this.srcDn;
	var target = (invert) ? this.srcDn : this.targetDn;
	var srcNm = (invert) ? this.getTargetName() : this.getSrcName();
	var targetNm = (invert) ? this.getSrcName() : this.getTargetName();
	if (mode == "message") {
	    return window.confirm("Add " + srcNm + " as member of " +
				  targetNm + "?" );
	}
    } else if (invert) {
	this.dndForm.dn.value = this.targetDn;
	this.dndForm['uniqueMember-1'].value = this.srcDn;
    } else {
	this.dndForm.dn.value = this.srcDn;
	this.dndForm['uniqueMember-1'].value = this.targetDn;
    }
}

function ps_dndChangeOwner(invert, mode) {
    this.dndForm.FormAction.value = "Modify";

    if (arguments.length > 1) {
	var src = (invert) ? this.targetDn : this.srcDn;
	var target = (invert) ? this.srcDn : this.targetDn;
	var srcNm = (invert) ? this.getTargetName() : this.getSrcName();
	var targetNm = (invert) ? this.getSrcName() : this.getTargetName();
	if (mode == "message") {
	    return window.confirm("Change owner of " + srcNm + " to " +
				  targetNm + "?" );
	}
    } else if (invert) {
	this.dndForm.dn.value = this.srcDn;
	this.dndForm['owner-1'].value = this.targetDn;
    } else {
	this.dndForm.dn.value = this.targetDn;
	this.dndForm['owner-1'].value = this.srcDn;
    }
}

function ps_dndModifyDN(invert, mode) {
    this.dndForm.FormAction.value = "modDNRequest";

    if (arguments.length > 1) {
	var src = (invert) ? this.targetDn : this.srcDn;
	var target = (invert) ? this.srcDn : this.targetDn;
	var srcNm = (invert) ? this.getTargetName() : this.getSrcName();
	var targetNm = (invert) ? this.getSrcName() : this.getTargetName();
	if (mode == "message") {
	    return window.confirm("Move " + srcNm + " to " + targetNm + "?");
	}
	else if ((mode == "fixTree" )  && (this.srcCtxt  == "orgTree") &&
		 (this.targetCtxt  == "orgTree") ) {
	    window.status = "Fixing organizational tree: " + src + ":" + target;
	    var el = ps_moveRowToNode(src, target);
	    if (null != el) {
		el.setAttribute('id',
				(src.replace(/([^=]*)=([^,]*),.*/,'$1=$2') +
				 "," + target).replace(/\s*,[ ]*\s*/g, ", ") );
		el.setAttribute('recordid', el.getAttribute('id') );
		el.lastChild.firstChild.setAttribute('dn',
						     el.getAttribute('id'));
		el.lastChild.firstChild.setAttribute('alt',
						     el.getAttribute('id'));
		el.lastChild.lastChild.setAttribute('href',
		    "javascript: void getEditableRecord('" +
		    el.getAttribute('id') + "','editFrame');");
	    } else if (ps_dndState.xslmgr) {
		ps_dndState.xslmgr.setXmXML();
	    } else {
		location.reload(true);
	    }
	}
    } else if (invert) {
	this.dndForm.dn.value = this.targetDn;
	this.dndForm.newrdn.value =
	    this.targetDn.replace(/([^=]*)=([^,]*),.*/,'$1=$2');
	this.dndForm.newSuperior.value = this.srcDn;
    } else {
	this.dndForm.dn.value = this.srcDn;
	this.dndForm.newrdn.value =
	    this.srcDn.replace(/([^=]*)=([^,]*),.*/,'$1=$2');
	this.dndForm.newSuperior.value = this.targetDn;
    }
}

function ps_dndSetSource(ev) {
    var myEvent = (arguments.length < 1) ? window.event : ev;
    var elmt = (myEvent.target) ? myEvent.target : myEvent.srcElement;
    ps_dndState.srcDn = elmt.getAttribute('dn');
    ps_dndState.srcOc = elmt.getAttribute('oc');
    ps_dndState.srcCtxt = elmt.getAttribute('dndCtxt');
    ps_dndState.dndForm = document.getElementById("dndChangeFrame").contentWindow.document.getElementById("dndChangeForm");
    if(null == ps_dndState.dndForm) document.getElementById("dndChangeDiv").style.display="block";
    /*
    try {
	with (window.clipboardData) {
	    setData("Text", ps_dndState.srcDn);
	}
    } catch (e1) {
	window.status = "window clipboardData is not supported" + e1;
    */
    try {
	with (myEvent.dataTransfer) {
	    effectAllowed = "copyLink";
	    dropEffect = "copy";
	    clearData();
	    if (setData("Text", ps_dndState.srcDn) ||
		setData("text/plain", ps_dndState.srcDn) )
		window.status = "Successfully set data to " + ps_dndState.srcDn; 
	}
    } catch(e2) {
	window.status = "event dataTransfer is not supported" + e2;
    }
/*    } */
    window.status = "Drag source is " + ps_dndState.srcCtxt + ":" +
	ps_dndState.srcDn;
}

function ps_dndSetTarget(ev) {
    var myEvent = (arguments.length < 1) ? window.event : ev;
    var elmt = (myEvent.target) ? myEvent.target : myEvent.srcElement;
    while (! elmt.getAttribute) elmt = elmt.parentNode;
    ps_dndState.targetDn = elmt.getAttribute('dn');
    ps_dndState.targetOc = elmt.getAttribute('oc');
    ps_dndState.targetCtxt = elmt.getAttribute('dndCtxt');

    window.status="Drop target is " + ps_dndState.targetCtxt + ":" +
	ps_dndState.targetDn;

    /* Ensure further handlers do not fire */
    if (myEvent.preventDefault) myEvent.preventDefault();
    else myEvent.returnValue = false;

    /* IE chokes on alerts and confirms in mid-event...*/
    window.setTimeout(function() { ps_dndHandleDragDrop(ps_dndState); }, 0);
}

function ps_dndCancelEvent(ev) {
    var myEvent = (arguments.length < 1) ? window.event : ev;
    if (myEvent.preventDefault) myEvent.preventDefault();
    else myEvent.returnValue = false;
    window.status = "Cancelled event...";
}

function ps_dndDeleteEmptyFormInputs(dndForm)
{
    var i;
    for (i = 0; i < dndForm.elements.length; i++) {
	if(dndForm.elements[i].value == "") {
	    dndForm.removeChild(dndForm.elements[i]);
	    i--;
	}
    }
}

function ps_dndHandleDragDrop(ds)
{
    var i;
    window.status = ds.srcDn + " dropped on " + ds.targetOc + " " + ds.targetDn;
    for (i = 0; i < ps_dndActionBindings.length; i++) {
	if ( ((ds.srcOc == ps_dndActionBindings[i].srcOc) ||
	      (ps_dndActionBindings[i].srcOc.length == 0) )
	     &&
	     ((ds.targetOc == ps_dndActionBindings[i].targetOc) ||
	      (ps_dndActionBindings[i].targetOc.length == 0) )
	     ) {
	    var action_args = [ ps_dndActionBindings[i].invert, "message" ];
	    window.status = "Performing DnD action " +
		ps_dndActionBindings[i].action.name;
	    if ((null != ps_dndActionBindings[i].action) &&
		(ps_dndActionBindings[i].action.apply(ds, action_args)) ) {
		var action_args = [ ps_dndActionBindings[i].invert ];
		ps_dndActionBindings[i].action.apply(ds, action_args);
		ps_dndDeleteEmptyFormInputs(ds.dndForm);
		/*
		document.getElementById("dndChangeDiv").style.display = "block";
		*/
		ds.dndForm.submit();
	    }
	    break;
	}
    }
    if ((i >= ps_dndActionBindings.length) ||
	(null == ps_dndActionBindings[i].action) ) {
	window.alert("Drag and drop not supported for this source and target");
    }
}

function ps_showResultsAndReinit()
{
    var myFrame = document.getElementById("dndChangeFrame");
    if (myFrame) {
	var respDocument = myFrame.contentWindow.document;
	var myResult = respDocument.getElementsByTagName("resultCode");
	if (myResult.length > 0) {
	    if ("0" == myResult[0].getAttribute("code")) {
		window.status = "Update was successful";
		var ds = ps_dndState;
		for (i = 0; i < ps_dndActionBindings.length; i++) {
		    if ( ((ds.srcOc == ps_dndActionBindings[i].srcOc) ||
			  (ps_dndActionBindings[i].srcOc.length == 0) )
			 &&
			 ((ds.targetOc == ps_dndActionBindings[i].targetOc) ||
			  (ps_dndActionBindings[i].targetOc.length == 0) )
			 ) {
			if ((null != ps_dndActionBindings[i].action)) {
			    var action_args = [ ps_dndActionBindings[i].invert,
						"fixTree" ];
			    window.status = "Performing DnD action " +
				ps_dndActionBindings[i].action.name;
			    ps_dndActionBindings[i].action.apply(ds,
								 action_args);
			}
		    }
		}
	    } else {
		var errMsg = respDocument.getElementsByTagName("errorMessage");
		var msg = (errMsg[0].text) ? errMsg[0].text :
		    errMsg[0].textContent;
		top.alert("Update failed: " + msg );
	    }
	    myFrame.contentWindow.document.location.href =
		psldapRootUri + "/dndForm.html";
	}
    }
}

function initDndProcessingForm()
{
    var f, e = document.createElement("div");
    e.setAttribute("id", "dndChangeDiv");
    e.style.display='none';
    document.body.appendChild(e);
    e.appendChild(f = document.createElement('iframe'));
    f.setAttribute("id", "dndChangeFrame");
    f.onload = ps_showResultsAndReinit;
    f.src = psldapRootUri + "/dndForm.html";
}

function attachDnDEventsForElementTypeWithDNAttr( theDoc, elmtType, myXslMgr)
{
    var elmts = theDoc.getElementsByTagName(elmtType);
    var i = elmts.length;
    
    if (i > 0) {
	initDndProcessingForm(ps_dndState);
    }

    if (arguments.length > 2) {
	ps_dndState.xslmgr = myXslMgr;
    }

    while( i-- > 0 ) { 
	var src = elmts[i];
	if (src.addEventListener) {
	    src.addEventListener("dragstart", ps_dndSetSource, false);
	    src.addEventListener("draggesture", ps_dndSetSource, false); // alternate to drag start
	    /*
	    src.addEventListener("drag", ps_dndCancelEvent, false);
	    src.addEventListener("dragend", ps_dndCancelEvent, false);
	    */
	    
	    src.addEventListener("dragenter", ps_dndCancelEvent, false);
	    src.addEventListener("dragover", ps_dndCancelEvent, false);
	    src.addEventListener("dragleave", ps_dndCancelEvent, false);
	    src.addEventListener("dragexit", ps_dndCancelEvent, false);
	    src.addEventListener("drop", ps_dndSetTarget, false);
	    src.addEventListener("dragdrop", ps_dndSetTarget, false);
	} else {
	    src.attachEvent("ondragstart", ps_dndSetSource);
	    /*
	    src.attachEvent("ondrag", ps_dndCancelEvent);
	    src.attachEvent("ondragend", ps_dndCancelEvent);
	    */
	    
	    src.attachEvent("ondragenter", ps_dndCancelEvent);
	    src.attachEvent("ondragover", ps_dndCancelEvent);
	    src.attachEvent("ondrop", ps_dndSetTarget);
	    src.attachEvent("ondragleave", ps_dndCancelEvent);
	}
    }
}
    
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
 *  @param c_value the value to set
 *  @return the unescaped value stored in the cookie
 **/
function ps_set_cookie ( c_name, c_value )
{
    var result = true;
    if (document.cookie) {
	;
    } else if (document.cookies) {
	;
    } else {
	window.status = "Cookies not accessible, personalization disabled";
	result = false;
    }
    return result;
}

/** Provides the value associate with name from loaded cookies
 *  @param c_name the cookie name
 *  @return the unescaped value stored in the cookie
 **/
function ps_get_cookie ( c_name )
{
    var c = null;
    var cookieStr = (document.cookie) ? document.cookie : document.cookies;
    if (cookieStr) {
	index = cookieStr.indexOf(c_name);
	if (index != -1) {
	    var v_start = (cookieStr.indexOf("=", index) + 1);
	    var v_end = cookieStr.indexOf(";", v_start);
	    if (v_end == -1) { v_end = cookieStr.length; }
	    c = cookieStr.substring(v_start, v_end);
	} else {
	    window.status = "Missing cookie " + c_name + ": <" + cookieStr + ">";
	}
    } else {
	window.status = "Cookies not accessible, personalization disabled";
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

/** This function should be assigned as a method on the input element bound to
 *  the email edit
 **/
function psldap_validateEMail() {
    var value = this.value;
    var result = value.indexOf("@");
    if (-1 != result) {
	result = value.indexOf(".", result + 2);
	if (-1 != result) {
	    result = value.length - result - 2;
	}
    }
    if (result <= -1) {
	this.psValidationMsg = "Poorly formatted email: " + value;
    }
    return (result > -1);
}

/** This function should be assigned as a method on the input element bound to
 *  the value.
 **/
function psldap_validateMinLength(minLength) {
    var value = this.value;
    var result = value.length >= minLength;
    if (!result) {
	this.psValidationMsg = "Value for " + this.name +
	    " is less than min length of " + minLength;
    }
    return result;
}

/** This function should be assigned as a method on the input element bound to
 *  the value.
 **/
function psldap_validateMaxLength(maxLength) {
    var value = this.value;
    var result = value.length >= maxLength;
    if (!result) {
	this.psValidationMsg = "Value for " + this.name +
	    " is greater than max length of " + maxLength;
    }
    return result;
}

/** This function should be assigned as a method on the input element bound to
 *  the value.
 **/
function psldap_validatePasswordStrength(_strength) {
    var value = this.value;
    var strength = (arguments.length < 1) ? 1 : _strength;
    var result = true;
    var msg = "";
    if (strength > 0) {
	result = (value.length >= 6);
	msg = "greater than 5 characters";
    }
    if (strength > 1) {
	if (false) {
	    result = false;
	    msg = "have letters AND numbers";
	}
    }
    if (strength > 2) {
	if (false) {
	    result = false;
	    msg = "have non-sequential numbers";
	}
    }
    if (strength > 3) {
	if (false) {
	    result = false;
	    msg = "have non-alphanumeric characters";
	}
	if (false) {
	    result = false;
	    msg = "have 8 or more characters";
	}
    }
    if (!result) {
	this.psValidationMsg = "Value for password must be: " +  msg;
    }
    return result;
}

/** Bound to the onsubmit action for the form - iterates across all input 
 *  elements and invokes the validations bound to psvalidate, returning false 
 *  if a validation fails and true otherwise
 **/
function psldap_validateFormInput() {
    var result = true;
    var errBuffer = "";
    var elmts = document.getElementsByTagName('input');
    var objForm = getFocusedFormElement();

    elmts = objForm.elements;

    var i = elmts.length;

    while ( --i >= 0) {
	var fr;
	var elmt = elmts[i];
	var psv_attr = elmt.getAttribute('psvalidate');

	if ((null != psv_attr) && ('' != psv_attr))  {
	    var psv_args = psv_attr.split(',');
	    var psv_func = psv_args.shift(); 
	    var msg = "";
	    elmt.psValidationMsg = "";
	    try {
		var f = new Function("return " + psv_func + ".apply(this, arguments);");
		result = ((fr = f.apply(elmt, psv_args)) && result);
		msg = "<li>" + elmt.psValidationMsg + "</li>";
	    }
	    catch(e1) {
		result = false;
		msg = "<li>Form validation flaw (" + e1 + ")detected on field " +
		    elmt.name + "</li>";
	    }
	    if (!fr) errBuffer = errBuffer + msg;
	}
    }
    if (!result) setProcessWindowMsg("<body style='background-color: #FFFFFF; font-family: arial; font-size: 14px' ><p>The following errors were encountered:</p><ul>" + errBuffer + "</ul></body>");

    return result;
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
    sizeWindowToFitDocument(window);
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

/**  This function is intended to be invoked when a page is loaded to set the
 *   selectable DNs to those indicated in the psldap_config.js file
 **/
function setOptionsOnAllDNSelects() {
    var dnElmt = document.getElementById("dn");
    if ((null != dnElmt) && (dnElmt.tagName.toUpperCase() == "SELECT") ) {
	for (var i=0; i < ldapDomains.length; i++) {
	    var optStr = '<option ' + ((ldapDomains[i].defaultDomain==1)?"selected ":"") + 'value="' + ldapDomains[i].dn + '">' + ldapDomains[i].label + '</option>';
	    if (0 == i) dnElmt.innerHTML = optStr;
	    else dnElmt.innerHTML += optStr;
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

function getFocusedFormElement() {
    var objForm = getAllFormElements();
    objForm = objForm[currentRecord-1];
    return objForm;
}

function refreshOpenerAndClose()
{
    window.opener.location.href = window.opener.location.href;
    window.close();
}

function authenticateViaBasic(userkey, password)
{
    try {
	var reqW = new RequestWrapper();
	reqW.init();
	reqW.open(psldapBaseUri + ldapupdateUri + "?FormAction=Search&search=(mail=zzz)&scope=base&xsl1=DSML_response.xsl", false, refreshOpenerAndClose, userkey, password);
    } catch(e1) {
	window.status("Page does not support (re)authentication via Basic");
    }
}

function postRegistrationResponse() {
    var result = 99;
    wt = document.getElementById("processWindow");
    if (null != wt) {
	var responseDoc = wt.contentWindow.document;
	var elmts = responseDoc.getElementsByTagName('resultCode');
	if ((null != elmts) && (elmts.length > 0) ) {
	    var l = elmts.length;
	    var ff = getFocusedFormElement();
	    var action = ff.FormAction.value;
	    while (--l >= 0) {
		result = elmts[l].getAttribute('code');
		dn = elmts[l].getAttribute('dn');
	    }
	    if ((("0" == result) && (action == "Register")) ||
		(("68" == result) &&
		 window.confirm("Account exists - would you like to login?"))
		 ) {
		authenticateViaBasic(ff.elements[ps_siteConfig.userKey+'-1'].value, ff.elements[ps_siteConfig.passKey+'-1'].value);
		/*ps_set_cookie( 'PsLDAPNextUri', 'statusPage.html');*/
		ff.target="_self";
		submitVisibleRecord("Login", false);
	    }
	} else {
	    window.status = "Status for last update unavailable";
	}
    }
    return result;
}

function setProcessWindowMsg(htmlStr) {
    var wt = document.getElementById("processWindow");
    if (null != wt) {
	wt.contentWindow.document.open();
	wt.contentWindow.document.write(htmlStr);
	wt.contentWindow.document.close();
    }
}

function resetProcessWindow() {
    var wt = document.getElementById("processWindow");
    if (null != wt) {
	wt.src = wt.src;
	/*
	wt.contentWindow.document.open();
	wt.contentWindow.document.write("<p><em>Processing request...</em></p>");
	wt.contentWindow.document.close();
	*/
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
function setRegistrationDNFromFormData(objForm) {
    var objCNElement = (objForm.elements["cn"]) ? objForm.elements["cn"] :
	objForm.elements["cn-1"];
    
    objForm.dn.value = getBaseDNFromMyUrl();
    if ("" == objForm.dn.value) {
	objForm.dn.value = prompt("Enter the base DN to contain this record");
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
    /*
      objForm.dn.value = window.prompt("Confirm the DN for this record:", objForm.dn.value);
    */
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

function moveVisibleRecord(action, doConfirm) {
    var objForm = getFocusedFormElement();
    var confirmSubmit = ((arguments.length < 2) || doConfirm);
    if (confirmSubmit) {
	if (objForm.elements["newrdn"]) {
	    objForm.newrdn.value = prompt("Enter the new RDN for this record",
					  objForm.newrdn.value);
	}
	if (objForm.elements["newSuperior"]) {
	    objForm.newSuperior.value = prompt("Enter the new superior for this record", objForm.newSuperior.value);
	}
    }

    if (objForm.elements["newrdn"] && objForm.elements["newSuperior"]) {
	submitVisibleRecord(action, confirmSubmit);
    } else {
	alert("New RDN and new Superior are not defined - aborting move request");
    }
}

/** Copies the visible form into an iframe labeled as "processWindow" in the
 *  current document and submits the cloned item, capturing the result in the
 *  iframe and maintaining the integrity of the pending form.
 *  @param action one of three string constants: Create, Delete, Update
 **/
function submitVisibleRecord(action, doConfirm) {
    var objForm = getFocusedFormElement();
    var userConfirmed = true;

    objForm.FormAction.value = action;

    if (action == "Register") {
	setRegistrationDNFromFormData(objForm);
    } else {
	if (action == "Create") { setDNFromFormData(objForm); }
	if((arguments.length < 2) || doConfirm) {
	    userConfirmed = window.confirm("Are you sure you wish to " +
					   action +
					   " this record?");
	}
    }
    
    if (userConfirmed ) {
	var objClone = objForm;
	/*
	  var wt = document.getElementById(objForm.target);
	  if (null != wt) { wt = wt.contentWindow; }
	  objClone = cloneFormToWindow(objForm, wt);
	*/
        showProcessDocument(true);

        if ((undefined == objClone.onsubmit) || objClone.onsubmit()) {
	    objClone.submit();
	}
    }
}

function resetVisibleRecord() {
    var objForm = getFocusedFormElement();
    objForm.reset();
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
    if ((objForm.length > 1) && (recordNbrElmt)) {
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
            objWindow = window.open(sel, "", "resizable=yes, menubar=no, toolbar=no, location=no, height=400, width=400");
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
    theMgr.addStylesheet("edittools", psldapRootUri + "/DSML_edittools.xsl");
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

function getWindowWidth(objWindow) {
    var myWidth = 0;

    if( objWindow.innerWidth && typeof( objWindow.innerWidth ) == 'number' ) {
        //Non-IE
        myWidth = objWindow.innerWidth;
    } else if( document.documentElement &&
	       ( undefined != document.documentElement.clientWidth ) &&
	       ( 0 < document.documentElement.clientWidth ) ) {
        //IE 6+ in 'standards compliant mode'
        myWidth = document.documentElement.clientWidth;
    } else if( document.body && ( undefined != document.body.clientWidth ) ) {
        //IE 4 / 5 compatible
        myWidth = document.body.clientWidth;
    }
    if (myWidth < 100) {
	window.alert ("Window width: " + objWindow.innerWidth + ", " + 
		      document.documentElement.clientWidth + ", " +
		      document.body.clientWidth);
	myWidth = 150;
    }
    return myWidth;
}

function getWindowHeight(objWindow) {
    var myHeight = 480;

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
	window.status = "Window height defaulting: " +
	    objWindow.innerHeight + ", " + 
	    document.documentElement.clientHeight + ", " +
	    document.body.clientHeight;
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

function windowIsAllOnScreen(wo) {
    var x = wo.screenX, y = wo.screenY;
    var de = wo.document.documentElement;
    var dw = de.scrollWidth - getWindowWidth(wo);
    var dh = de.scrollHeight - getWindowHeight(wo);

    if (x == undefined) x = wo.screenTop;
    if (y == undefined) x = wo.screenLeft;

    return (wo.screen.width > (x + dw)) && (wo.screen.height > (y + dh));
}

function sizeWindowToFitDocument(wo) {
    /* If the window is not embedded, resize to fit the form */
    if (wo.top == wo) {
	var de = wo.document.documentElement;
	/*
	var width = de.scrollWidth - de.clientWidth;
	var height = de.scrollHeight - de.clientHeight;
	*/
	var width = de.scrollWidth - getWindowWidth(wo);
	var height = de.scrollHeight - getWindowHeight(wo);
 
	wo.resizeBy(width, height);
	if (!windowIsAllOnScreen(wo)) {
	    wo.moveTo((wo.screen.width - de.clientWidth) / 2,
		      (wo.screen.height - de.clientHeight) / 2);
	}
    }
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

function sortByTreeParentId(objItem1, objItem2)
{
    if (!objItem1.parentElmtCount) {
        objItem1.parentElmtCount = objItem1.treeParentId.match(/=/gi).length;
    }
    if (!objItem2.parentElmtCount) {
        objItem2.parentElmtCount = objItem2.treeParentId.match(/=/gi).length;
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
	window.status = "Tree " + tableIdStr + " not found in document";
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
        var managerTree = (0 == parentDelimStr.indexOf("refattr"));

	objTable.mode = (managerTree) ? "manager" : "org" ;
        for (var i = 0; i < objElmtArray.length; i++) {
            var manager = null;
            var reNormal = /\s*,[ ]*\s*/g;

            objRecord = objElmtArray[i];
            if (managerTree) {
                objRecord.treeParentId = objRecord.getAttribute("refattr");
		objRecord.setAttribute("refattr",
		   objRecord.getAttribute("refattr").replace(reNormal, ", "));
            } else  {
                objRecord.treeParentId =
		    objRecord.getAttribute(rowIdStr).replace(parentRegex, "");
            }

            // Assign parent and normalize white space in id attributes
            objRecord.treeParentId = objRecord.treeParentId.replace(reNormal, ", ");
            objRecord.setAttribute(rowIdStr,
                   objRecord.getAttribute(rowIdStr).replace(reNormal, ", "));

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
                    var objTableElmt = document.createElement("table");
                    var objTableBody = document.createElement("tbody");
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
                    }
                }
            }
        }
        addNavigationToTree(tableIdStr);
	window.status = "Tree " + tableIdStr + " is fully loaded";
    }
}
