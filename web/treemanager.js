/*
 * mod_psldap
 *
 * User Authentication against and maintenance of an LDAP database
 *
 * Copyright (C) 2004-2024 PSInd, LLC
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
/*************************************************************************
 * Tree navigation functions
 *************************************************************************
 */

var tmBaseUrl = "";
try {
    if (urlBase.length > 0) tmBaseUrl = urlBase;
} catch (ex) {
    window.status = "Using psajax treemanager...";
}

function generateEmptyNodeString() {
    return "<img style='width: 12px; height: 12px; margin-left: 2px;' src='"+tmBaseUrl+"/images/nconnect.gif' alt=' ' />";
}

function generateExpandNodeString(navNode, treeNode) {
    return "<a style=\"border-width: 0px; text-decoration: none;\" href=\"javascript:void(0);\" onClick=\"expandTreeNode(\'" + navNode.id + "\',\'" + treeNode.id + "\');\"><img style='width: 12px; height: 12px;' src='"+tmBaseUrl+"/images/ebutton.gif' alt='+' /></a>";
}

function generateCollapseNodeString(navNode, treeNode) {
    return "<a style=\"border-width: 0px; text-decoration: none; \" href=\"javascript:void(0);\" onClick=\"collapseTreeNode(\'" + navNode.id + "\',\'" + treeNode.id + "\');\"><img style='width: 12px; height: 12px;' src='"+tmBaseUrl+"/images/cbutton.gif' alt='-' /></a>";
}

function collapseTreeNode(navNodeId, treeNodeId) {
  var treeNode = document.getElementById(treeNodeId);
  var navNode = document.getElementById(navNodeId);
  if ( (null == treeNode) || (null == navNode) ) {
    throw new Error("Failed to collapse tree element: " + treeNodeId);
  }
  treeNode.style.display="none";
  navNode.innerHTML = generateExpandNodeString(navNode, treeNode);
}

function expandTreeNode(navNodeId, treeNodeId) {
  var treeNode = document.getElementById(treeNodeId);
  var navNode = document.getElementById(navNodeId);
  if ( (null == treeNode) || (null == navNode) ) {
    throw new Error("Failed to expand tree element: " + treeNodeId);
  }
  treeNode.style.display="block";
  navNode.innerHTML = generateCollapseNodeString(navNode, treeNode);
}

/**
 * This gets the table row element containing the passed table row element
 * within an embedded table.
 *
 * @param tableRowElement - the <tr> element whose containing <tr> we wish
 *        to find.
 **/
function getNavParentForNode(tableRowElement) {
  // parent == table, parent.parent == td, parent.parent.parent == tr
  var result = tableRowElement.parentNode;

  while ((null != result) && (!result.id || (-1 == result.id.indexOf("tntable")) ) ) {
    result = result.parentNode;
  }
  if (null != result) {
    result = result.parentNode.parentNode;
  }
  // We really need to test for a table row cell here
  return result;
}

function getNavElementForNode(tableElement) {
  var result = tableElement.parentNode;
  // We really need to test for a table cell here
  if (undefined != result.parentNode.firstElementChild) {
      result = result.parentNode.firstElementChild;
  } else {
      result = result.parentNode.firstChild;
  }
  return result;
}

function getTreeNodeByLabel(label) {
  var result = getTreeNodeByLabel2(label);
  //if (!result) result = document.getElementById(label);
  return result;
}

function getTreeNodeByLabel2(label) {
  var anchorList = document.body.getElementsByTagName("A");
  var result = null;

  for (var i = 0; !result && (i < anchorList.length); i++) {
      var childList = "Node children: " + anchorList[i].childNodes.length;
      for (var j = 0; j < anchorList[i].childNodes.length; j++) {
	  var alcn = anchorList[i].childNodes[j];
	  childList += "  type = " + alcn.nodeType + ": " + alcn.nodeValue;
	  if ((alcn.nodeType == 3) && (alcn.nodeValue == label) ) {
	      result = anchorList[i];
	      break;
	  }
      }
  }

  return result;
}

function expandTreeHierarchy(treeNodeId) {
  // This gets the nav cell of the row element containing the table -
  //     the element type should be verified.
  var expandnode = getTreeNodeByLabel(treeNodeId);
  var result = expandnode;
  if (null != expandnode) {
    for (var expandnode = getNavParentForNode(expandnode); null != expandnode;
         expandnode = getNavParentForNode(expandnode) ) {
	if (undefined != expandnode.firstElementChild) {
	    expandnode.firstElementChild.firstElementChild.onclick();
	} else {
	    expandnode.firstChild.firstChild.onclick();
	}
    }
  }
  return result;
}

function collapseAllTreeNodes(rootNodeId) {
  var rootnode = (typeof(rootNodeId) != "string") ? rootNodeId :
      document.getElementById(rootNodeId);
  if (null == rootnode) throw new Error("Root of tree not found: " + rootNodeId);
  var nodeList = rootnode.getElementsByTagName("TABLE");
  for (var i = 0; i < nodeList.length; i++) {
    var navElement = getNavElementForNode(nodeList[i]);
    var navAnchor = navElement.getElementsByTagName("A");
    navAnchor[0].onclick();
  }
}

function runAnchorHref(selectedNode) {
  selectedNode.focus();
  var jsIndex = selectedNode.href.indexOf(":");
  if (jsIndex > -1) {
    var jsString = selectedNode.href.substring(jsIndex + 1);
    var tempFunc = new Function(jsString);
    tempFunc();
  } else {
    alert("failed to run href (" + selectedNode.nodeType + ") from index " + jsIndex + ": " + selectedNode.href);
  }
}

function pstm_getAllRowsOfTable(objTable)
{
    var result = objTable.getElementsByTagName("TR");
    var next_tr = objTable.lastChild;
    while (next_tr && (!next_tr.tagName || (0 != next_tr.tagName.indexOf("TR")) ) ) {
        next_tr = next_tr.lastChild;
	while (next_tr.nodeType != 1) next_tr = next_tr.previousSibling;
    }

    result = new Array();
    while (next_tr) {
	result.unshift(next_tr);
	for (next_tr = next_tr.previousSibling; next_tr && (next_tr.nodeType != 1);
	     next_tr = next_tr.previousSibling);
    }

    return result;
}

function pstm_findOrCreateNextLevel(el)
{
    var result = null;

    el = el.lastChild;
    if (0 != el.lastChild.tagName.indexOf("TABLE")) {
	var objTableElmt = document.createElement("table");
	var objTableBody = document.createElement("tbody");

	var alltns = document.getElementsByName("treeNodeTable");
	for (var i = alltns.length; i >= 0; i--) {
	    if ( ! document.getElementById('tntable'+i)) {
		objTableElmt.id = 'tntable'+i;
		objTableElmt.setAttribute('id','tntable'+i);
	    }
	}
	objTableBody.style.width = "100%";
	objTableElmt.setAttribute('name','treeNodeTable');
	objTableElmt.style.width = "100%";
	objTableElmt.appendChild(objTableBody);
	el.appendChild(document.createElement("BR"));
	el.appendChild(objTableElmt);
	addNavigationToTree(objTableElmt);
	
	result = objTableBody;

	var navElmt = getNavElementForNode(objTableElmt);
	var allnes = document.getElementsByName("treeNavigationElement");
	for (var i = allnes.length; i >= 0; i--) {
	    if ( ! document.getElementById('nav'+i)) {
		navElmt.id ='nav'+i;
		navElmt.setAttribute('id','nav'+i);
	    }
	}
	navElmt.setAttribute('name', "treeNavigationElement");
	navElmt.style.verticalAlign="top";
	navElmt.style.width="12px";
	navElmt.innerHTML = generateCollapseNodeString(navElmt, objTableElmt);
    } else {
	result = el.lastChild.lastChild;
    }

    return result;
}

function pstm_getRecordRow(nodeId, _attrName)
{
    var result = null;
    var elmts = document.getElementsByName("TreeNode");
    var i;
    var idStr = nodeId.replace(/\s/g, "");
    var attrName = (arguments.length > 1) ? _attrName : "tnid";
    for(i = 0; i < elmts.length; i++) {
	if(0 == elmts[i].getAttribute(attrName).replace(/\s/g, "").localeCompare(idStr)) {
	    result = elmts[i];
	    break;
	}
    }
    return result;
}

function pstm_moveRowToNode(nodeId, newNodeId, _attrName, sortFunc)
{
    var attrName = (arguments.length > 2) ? _attrName : 'tnid';
    var srcElmt = pstm_getRecordRow(nodeId, attrName);
    var targetElmt = pstm_getRecordRow(newNodeId, attrName);

    if (srcElmt && targetElmt) {
	var childTbl = pstm_findOrCreateNextLevel(targetElmt);
	if ("true" == srcElmt.getAttribute("lastnode")) {
	    var prvSib = srcElmt.previousSibling;
	    if (prvSib) { prvSib.setAttribute("lastnode", "true"); }
	    else {
		var navElmt = srcElmt.parentNode;
		while (navElmt && (0 != navElmt.tagName.indexOf("TABLE"))) {
		    navElmt = navElmt.parentNode;
		}
		if (navElmt) {
		    var tableNode = navElmt;
		    navElmt = getNavElementForNode(navElmt);
		    navElmt.innerHTML = generateEmptyNodeString();
		    tableNode.parentNode.removeChild(tableNode.previousSibling);
		    tableNode.parentNode.removeChild(tableNode);
		}
	    }
	    srcElmt.removeAttribute("lastnode");
	}
	srcElmt.parentNode.removeChild(srcElmt);
	var ns = (undefined != childTbl.firstElementChild) ? childTbl.firstElementChild:childTbl.firstChild;
	if (arguments.length > 3) {
	    while (ns && (0 > sortFunc(srcElmt,ns))) {
		ns = ns.nextSibling;
	    }
	}
	if (ns) childTbl.insertBefore(srcElmt, ns);
	else {
	    childTbl.appendChild(srcElmt);
	    srcElmt.setAttribute("lastnode", "true");
	}
	var currDS = childTbl.style.display;
	childTbl.style.display = 'none';
	childTbl.style.display = currDS;
    } else {
	window.status = "Moved row "+ srcElmt +" to " + targetElmt;
    }
    return srcElmt;
}

/**
 * This method adds tree navigation capabilities to a recursive HTML table
 * based structure. This assumes that each row in the table maintains two
 * columns, the first contains nothing but non-breaking space and is a place
 * holder for the tree nav mechanism - the second column contains the label.
 * In addition to the label, the second column may also contain another
 * embedded table of the same form following the label, provided the label
 * is followed by a <br> tag.
 *
 * The tree is immediately collapsed after the navigation elements have
 * been added to the table. The user may optionally indicate one branch for
 * expansion by passing the id of the <tr> to show as the second paramter.
 *
 * @param nodeId - the HTML id of the table representing the root of the tree.
 * @param selectedNodeId - (optional) the id of the row whose label is to be
 *        made visible after the navigation is added.
 * @param checkBrowserArgs - is a boolean value where true indicates the URL
 *        should be checked for an argument, album, whose value will override
 *        the default selectedNodeId
 **/
function addNavigationToTree(nodeId, selectedNodeId,checkBrowserArgs, nodeName) {
    var expandNodeId = selectedNodeId;
    var rootnode = (typeof(nodeId) != "string") ? nodeId :
	document.getElementById(nodeId);
    if (null == rootnode) throw new Error("Root of tree not found: " + nodeId);
    
    rootnode.className = "pstree";
    rootnode.setAttribute('tnid', 'tn.root');

    var hNode = document.getElementsByTagName("head")[0];         
    var cssNode = document.createElement('link');
    cssNode.type = 'text/css';
    cssNode.rel = 'stylesheet';
    cssNode.href = tmBaseUrl + '/treemanager.css';
    cssNode.media = 'screen';
    hNode.appendChild(cssNode);

    var nodeList = rootnode.getElementsByTagName("TABLE");
    for (var i = 0; i < nodeList.length; i++) {
	nodeList[i].id = "tntable" + i;
	nodeList[i].setAttribute('name', "treeNodeTable");
	var navElement = getNavElementForNode(nodeList[i]);
	navElement.id="nav" + i;
	navElement.setAttribute('name',"treeNavigationElement");
	navElement.style.verticalAlign="top";
	navElement.style.width="12px";
	navElement.innerHTML = generateCollapseNodeString(navElement, nodeList[i]);
	var navRows = pstm_getAllRowsOfTable(nodeList[i]);
	for (var j = 0; j < navRows.length; j++) {
	    navRows[j].setAttribute('tnid', 'tn.'+i+'.'+j);
	    var nrName = navRows[j].getAttribute('name');
	    var nrtd = (undefined != navRows[j].firstElementChild) ? navRows[j].firstElementChild:navRows[j].firstChild;
	    while (nrtd && (nrtd.nodeType != 1)) nrtd = nrtd.nextSibling;
	    if (!nrName || (0 != nrName.localeCompare("treeNavigationElement") ) ) {
		if (nrtd) nrtd.innerHTML = generateEmptyNodeString();
	    }
	    for (nrtd = nrtd.nextSibling; nrtd && (nrtd.nodeType != 1);
		 nrtd = nrtd.nextSibling );
	    if (nrtd && !nrtd.getAttribute('tnclass'))
		nrtd.setAttribute('tnclass', 'misc');
	    navRows[j].setAttribute("name", "TreeNode");
	}
	if (navRows.length > 0) {
	    navRows[navRows.length-1].setAttribute("lastnode", "true");
	}
    }
    collapseAllTreeNodes(rootnode);
    if (arguments.length > 2) {
	try {
	    requestArgList = top.document.URL.split("?");
	} catch (ex) {
	    requestArgList = "";
	    alert(requestArgList);
	}
	if (requestArgList.length > 1) {
	    var passedNodeId = "";
	    requestArgList = requestArgList[1].split("&");
	    for (var i = 0; i < requestArgList.length; i++) {
		var thisArg = decodeURIComponent(requestArgList[i]);
		if ((0 == thisArg.indexOf("node=")) && (thisArg.length > 5) ) {
		    passedNodeId = thisArg.split("=")[1];
		    break;
		}
	    }
	    if (passedNodeId != "") {
		expandNodeId = passedNodeId;
	    }
	}
    }
    if (arguments.length > 1) {
	var selectedNode = expandTreeHierarchy(expandNodeId);
	if (null == selectedNode) {
	    selectedNode = expandTreeHierarchy(selectedNodeId);
	}
	runAnchorHref(selectedNode);
    }
}
