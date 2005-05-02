var isNav, isIE;

if (parseInt(navigator.appVersion) >= 4) {
    if (navigator.appName == "Netscape" ) {
        isNav = true;
    } else {
        isIE = true;
    }
}

// Update the URI with the bound URIs in the httpd.conf
//var psldapRootUri = document.URL.substring(0, document.URL.lastIndexOf("/"));
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
        objForm.dn.value = wt.prompt("Confirm the DN for this record:", objForm.dn.value);
    }
    
    if (wt.confirm("Are you sure you wish to change this record?") ) {
	if (!(wt.document.importNode) ||
             (objForm.encoding == "multipart/form-data") ) {
            /* IE does not support import node - the work arounds seem to
               be deficient when used with forms, so just submit the form
               in the context of the current page. Also multipart forms
	       tend to have file input elements which cannot be cloned. */
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

function toggleClassInfo() {
    alert("Class info modification not yet supported");
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

function loadRecordUrl(sel, target) {
    var objWindow = null;
    if (sel != "") {
        if ((arguments.length < 2) || (null == target)) {
            objWindow = window.open(sel, "", "resizable=yes, menubar=no, toolbar=no, height=739, width=669");
        } else {
            var objFrame = document.getElementById(target);
            objFrame.src = sel;
        }
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

function getEditableRecord(dn, target) {
    var getUrl = ldapupdateUri + "?FormAction=Search&" +
	"search=(objectClass=*)&dn=" + dn +
        "&BinaryHRef=on" +
        "&xsl1=" + psldapRootUri + "/DSML_editform.xsl" +
        "&xsl2=" + psldapRootUri + "/DSML_cards.xsl";
    loadRecordUrl(getUrl, target);
}

function getWindowHeight(objWindow) {
    var myHeight = 0;
    if( typeof( objWindow.innerWidth ) == 'number' ) {
        //Non-IE
        myHeight = objWindow.innerHeight;
    } else if( document.documentElement &&
              ( document.documentElement.clientWidth ||
                document.documentElement.clientHeight ) ) {
        //IE 6+ in 'standards compliant mode'
        myHeight = document.documentElement.clientHeight;
    } else if( document.body && ( document.body.clientWidth || document.body.clientHeight ) ) {
        //IE 4 compatible
        myHeight = document.body.clientHeight;
    }
    return myHeight;
}

function verticalWrapChildren(objCellElmt, reservedSize)
{
    var nextChild;
    /* Take 20 off the window height for the scrollbar thickness ... */
    var scrollThickness = 25;
    var wrapHeight = getWindowHeight(window) - scrollThickness - reservedSize;
    var currentElmt = objCellElmt;
    var currentElmtHeight = currentElmt.scrollHeight;
    var prevWrapHeight = objCellElmt.scrollHeight;
    var previousHeight = objCellElmt.scrollHeight;
    var brString = new String("BR");

    do {
        if ((currentElmtHeight >= wrapHeight) &&
            (currentElmt.firstChild != currentElmt.lastChild) ) {
            if (currentElmt != objCellElmt) {
                /* Shift the previously moved element back and remove any
                   trailing breaking space */
                nextChild = currentElmt.removeChild(currentElmt.lastChild);
                if (0 != brString.indexOf(nextChild.tagName) ) {
                    objCellElmt.insertBefore(nextChild, objCellElmt.firstChild);
                }
                while (0 == brString.indexOf(currentElmt.lastChild.tagName) ) {
                    currentElmt.removeChild(currentElmt.lastChild);
                }
            }

            currentElmt = document.createElement(objCellElmt.tagName);
            currentElmt.name = "wrapElmt";

            currentElmt.className = objCellElmt.className;
            //psldapCopyAttributes(currentElmt, objCellElmt);

            objCellElmt.parentNode.insertBefore(currentElmt, objCellElmt);

            prevWrapHeight = objCellElmt.scrollHeight;
        }
	previousHeight = objCellElmt.scrollHeight;

	if (currentElmt == objCellElmt) {
            previousHeight -= 1;
        } else {
            nextChild = currentElmt.appendChild(objCellElmt.firstChild);
            if (currentElmt.scrollHeight > previousHeight) {
                currentElmtHeight = currentElmt.scrollHeight;
                previousHeight = objCellElmt.scrollHeight;
            } else {
                currentElmtHeight = prevWrapHeight - objCellElmt.scrollHeight;
            }
        }
    } while ( (null != objCellElmt.firstChild) &&
              (objCellElmt.scrollHeight <= previousHeight ) );
}

function initializeCards(reservedSize) {
    window.status = "Organizing cards...";

    var cardTable = document.getElementById("cardTable");
    var cardTd = cardTable;
    while ((null != cardTd) && (0 != cardTd.tagName.indexOf("TD"))) {
        cardTd = cardTd.firstChild;
    }
    if (null != cardTd) {
        verticalWrapChildren(cardTd, reservedSize);
    }

    window.status = "Done";
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

    if (null != objTable) {
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
