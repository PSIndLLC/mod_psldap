var isNav, isIE

if (parseInt(navigator.appVersion) >= 4) {
    if (navigator.appName == "Netscape" ) {
        isNav = true;
    } else {
        isIE = true;
    }
}

var currentRecord = 0;
var formElmts = null;
var recordNbrElmt = null;

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
    var wt = document.getElementById("processWindow");
    var bForce = false;
    if (arguments.length > 0) bForce = bForceShow; 

    if (null == wt) {
        wt = parent.document.getElementById("processWindow");
    }
    if (null != wt) {
        var objXml = wt.contentWindow.document.getElementsByTagName("XML");
        if (bForce || (0 < objXml.length) ) {
            wt.height = "35px";
            wt.style.display = "block";
        } else {
            wt.style.display = "none";
        }
    }
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
    for (i = objNode.attributes.length - 1; i >= 0; i--) {
        var objAttr = objNode.attributes.item(i);
        result.setAttribute(objAttr.name, objAttr.value);
    }
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
        wt = wt.contentWindow;
    }
    
    objForm = objForm[currentRecord-1];
    objForm.FormAction.value = action;
    if (action == "Create") {
        objForm.dn.value = prompt("Enter the base DN to contain this record");
        if (objForm.elements["o"]) {
	    objForm.dn.value = "o=" + objForm.elements["o"].value + ", " + objForm.dn.value;
        }
        else if (objForm.elements["o-1"]) {
	    objForm.dn.value = "o=" + objForm.elements["o-1"].value + ", " + objForm.dn.value;
        }
        if (objForm.elements["ou"]) {
            objForm.dn.value = "ou=" + objForm.elements["ou"].value + ", " + objForm.dn.value;
        }
        else if (objForm.elements["ou-1"]) {
            objForm.dn.value = "ou=" + objForm.elements["ou-1"].value + ", " + objForm.dn.value;
        }
        if (objForm.cn) {
	    if (objForm.elements["givenName"] && objForm.elements["sn"] ) {
                objForm.cn.value = objForm.elements["givenName"].value + " " +
                    objForm.elements["sn"].value;
            }
            else if (objForm.elements["givenName-1"] && objForm.elements["sn-1"] ) {
	        objForm.cn.value = objForm.elements["givenName-1"].value + " " +
		  objForm.elements["sn-1"].value;
            }
            objForm.dn.value = "cn=" + objForm.cn.value + ", " + objForm.dn.value;
        }
        objForm.dn.value = wt.prompt("Confirm the DN for this record:", objForm.dn.value);
    }
    
    if (wt.confirm("Are you sure you wish to change this record?") ) {
        if (!(wt.document.importNode)) {
            /* IE does not support import node - the work arounds seem to
               be deficient when used with forms, so just submit the form
               in the context of the current page */
            /*wt.document.importNode = psldapImportNode;*/
	    objForm.submit();
            return;
        }
        var objClone = wt.document.importNode(objForm, true);
        var objBody = wt.document.getElementsByTagName("BODY")[0];
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
        showProcessDocument(true);
        objClone.submit();
    }
}

function resetVisibleRecord() {
    var objForm = getAllFormElements();
    objForm[currentRecord-1].reset();
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

function pvt_setRecordDisplay(aForms, position, displayStyle)
{
    var myRow = null;
    if ((position > 0) && (position <= aForms.length)) {
        myRow = getMyRow(aForms[position-1]);
    }
    if (null != myRow) {
        if (myRow.style.display != displayStyle) {
            myRow.style.display = displayStyle;
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

function initialize() {
    window.status = "Loading forms...";

    recordNbrElmt = document.getElementById("recordNumber");

    showRecord(1);
    window.status = getAllFormElements().length + " records loaded";
}

