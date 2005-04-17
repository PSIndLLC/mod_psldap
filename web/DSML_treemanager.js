/*************************************************************************
 * Tree navigation functions
 *************************************************************************
 */

function generateExpandNodeString(navNode, treeNode) {
  return "<a class=\"plainlink\" style=\"border-width: 2px; border-style: outset; background-color: blue; color: white;\" href=\"javascript:void(0);\" onClick=\"expandTreeNode(\'" + navNode.id + "\',\'" + treeNode.id + "\');\">+</a>"
  //return "<span onClick=\"expandTreeNode(\'" + navNode.id + "\',\'" + treeNode.id + "\');\">+</span>";
}

function generateCollapseNodeString(navNode, treeNode) {
  return "<a class=\"plainlink\" style=\"border-width: 2px; border-style: outset; background-color: blue; color: white;\" href=\"javascript:void(0);\" onClick=\"collapseTreeNode(\'" + navNode.id + "\',\'" + treeNode.id + "\');\">-</a>"
  //return "<span onClick=\"collapseTreeNode(\'" + navNode.id + "\',\'" + treeNode.id + "\');\">-</span>";
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
  result = result.parentNode.firstChild;
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
      childList += "  type = " + anchorList[i].childNodes[j].nodeType + ": " + anchorList[i].childNodes[j].nodeValue;
      if ((anchorList[i].childNodes[j].nodeType == 3) &&
          (anchorList[i].childNodes[j].nodeValue == label) ) {
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
      expandnode.firstChild.firstChild.onclick();
    }
  }
  return result;
}

function collapseAllTreeNodes(rootNodeId) {
  var rootnode = document.getElementById(rootNodeId);
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
function addNavigationToTree(nodeId, selectedNodeId,checkBrowserArgs) {
  var expandNodeId = selectedNodeId;
  var rootnode = document.getElementById(nodeId);
  if (null == rootnode) throw new Error("Root of tree not found: " + nodeId);
  var nodeList = rootnode.getElementsByTagName("TABLE");
  for (var i = 0; i < nodeList.length; i++) {
    nodeList[i].id = "tntable" + i;
    var navElement = getNavElementForNode(nodeList[i]);
    navElement.id="nav" + i;
    navElement.name="treeNavigationElement"
    navElement.style.verticalAlign="top";
    navElement.innerHTML = generateCollapseNodeString(navElement, nodeList[i]);
  }
  collapseAllTreeNodes(nodeId);
  if (arguments.length > 2) {
    requestArgList = top.document.URL.split("?");
    if (requestArgList.length > 1) {
      var passedNodeId = "";
      requestArgList = requestArgList[1].split("&");
      for (var i = 0; i < requestArgList.length; i++) {
        var thisArg = decodeURIComponent(requestArgList[i]);
        if ((0 == thisArg.indexOf("album=")) && (thisArg.length > 6) ) {
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
