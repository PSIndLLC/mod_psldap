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

/*  Instructions for use of psajax_docmgr:
 *  
 *  Include the psajax_docmgr.js script in your web page to leverage the 
 *  capabilities of any DocumentManager class in your application. The highest
 *  level class is the XSLManager. To use the XSL Manager, follow these steps:
 *  
 *  1.  Create a XSLManager using new XSLManager();
 *  2.  Set the templates on the XSLManager with the addStylesheet function
 *      bound to the XSLManager object.
 *  3.  Add any parameters required for transformation using the addMgrParameter
 *      function bound to the XSLManager object.
 *  4.  Set the XML to be transfered by specifying the generating URL through 
 *      the setXmXML method bound to the XSLManager object
 *  5.  Call the transform method bound to the XSLManager object to force a
 *      transformation to an HTML fragment to be written into the specified id 
 *      from append_to_id in the passed document specified as doc against a
 *      DOM node returned by the passed getXMLNodeFunc function to that returns 
 *      the appropriate node to be transformed from the loaded XML DOM, which is
 *      passed as a parameter to the getXMLNodeFunc function.
 */

function docmgr_unsupported(txt) {
    alert("DOM functionality not implemented to support this script " + txt);
}

function IFrameRequest()
{
    this.uri = "";
    this.open = docmgr_unsupported;
    this.onreadystatechange = docmgr_unsupported;
    this.asynch = false;
}

function RequestWrapper()
{
    this.init();
}

RequestWrapper.prototype.initActiveX = function()
{
    window.status = "Using activex controls in psajax_docmgr.js";
    try {
        window.status = "Trying msxml2.xmlhttp.3.0";
        this.reqObj = new ActiveXObject("Msxml2.XMLHTTP.3.0");
    } catch(e1) {
        try {
            window.status = "Trying microsoft.xmlhttp";
            this.reqObj = new ActiveXObject("Microsoft.XMLHTTP");
        } catch(e2) {
            window.status = "Trying iframe request object - XMLHTTP failed: " + e1.message;
            this.reqObj = new IFrameRequest();
        }
    }
};

RequestWrapper.prototype.initDOMImpl = function()
{
    window.status = "Using document implementation in psajax_docmgr.js";
    if (typeof XMLHttpRequest != 'undefined') {
        this.reqObj = new XMLHttpRequest();
    } else {
        this.reqObj = new IFrameRequest();
    }
};

RequestWrapper.prototype.init = function()
{
    this.location = null;
    if (window.ActiveXObject) {
        this.initActiveX();
    } else if (document.implementation && document.implementation.createDocument) {
        this.initDOMImpl();
    } else {
        this.reqObj = new DummyDOM();
    }
    this.getResponse = function() {
	return ((undefined != this.reqObj.responseXML) ? this.reqObj.responseXML : 
		(((undefined != this.reqObj.documentElement) && (undefined != this.reqObj.documentElement.xml)) ? this.reqObj.documentElement.xml :
		 this.reqObj.responseText) );
    }
};

RequestWrapper.prototype.open = function(docref, async, onload, user, passwd)
{
    this.location = docref;
    this.readyload = onload;
    try {
	this.reqObj.async = async;
	this.reqObj.validateOnParse = false;
	// For some reason this is failing with the freethreadeddomdocument...
	if ((undefined == this.reqObj.load) ||
	    (!this.reqObj.load(docref))){ 
	    throw("load undefined on request object");
	} else {
	    window.status = "Loaded doc " + docref;
	}
    } catch(e1) {
	window.status = "Exception loading doc: " + e1.message;
	if (arguments.length < 5) {
	    this.reqObj.open("GET", docref, false);
	} else {
	    this.reqObj.open("GET", docref, false, user, passwd);
	}
	this.reqObj.send(null);
	window.status = "Pending onload... ";
    }
    onload.call(this, null);
    window.status = "Done";
}

RequestWrapper.prototype.reload = function() {
    this.open(this.location, false, this.readyload);
}

DocumentManager.prototype = new RequestWrapper();
DocumentManager.prototype.reqObj = null;
DocumentManager.prototype.setAsynchronous = function(asynch) {
    if (undefined != this.reqObj.asynch) {
        this.reqObj.asynch = asynch;
    } else {
        alert("Synchronous acquisition of data is not supported by " +
              "your browser. You may experience unexpected behavior");
    }
};
DocumentManager.prototype.onload = null;
DocumentManager.prototype.loadinfo = null;
DocumentManager.superclass = RequestWrapper.prototype;

function DocumentManager(docref, onload, load_info, is_html)
{
    window.status = "Creating document manager... " + docref + ":" + load_info;
    this.init();
    this.loadinfo = load_info;
    this.readyload = onload;

    if (docref != null) {
        window.status = "Setting on ready state change for " + docref;
        if (undefined != this.reqObj.onload) {
            this.reqObj.onload = onload;
        } else {
            this.reqObj.onreadystatechange = function(evt) {
                window.status = "State change: " + this.readyState + ":" + this.status;
                if ((this.readyState == 4) && (this.status == 200) ) {
                    onload();
                }
            }
        }
	this.open(docref, false, onload);
    }
}

XSLDocumentManager.prototype = new DocumentManager(null, null, null, false);
XSLDocumentManager.prototype.initActiveX = function()
{
    try {
        window.status = "Trying MSXML2.FreeThreadedDOMDocument";
        this.reqObj = new ActiveXObject("MSXML2.FreeThreadedDOMDocument");
    } catch(e1) {
        window.status = "Trying iframe request object - freethreaded dom load failed: " + e1.message;
        this.reqObj = new IFrameRequest();
    } 
};

function XSLDocumentManager(xslid,uri,onload,load_info)
{
    this.style_id = xslid;
    this.base = DocumentManager
    this.base(uri, onload, load_info);
}

function LoadXSLOnManager(e)
{
    this.loadinfo.txstyles[this.style_id] = this;
    var resp = this.getResponse();
    window.status = "Loaded xsl processed ..." + resp;
}

function LoadXMLOnManager(e)
{
    var resp = this.getResponse();
    this.loadinfo.xmldoc = resp;
    window.status = "Loaded xml processed ..." + resp;
}

function XMLTransformData(mgr, xsl_id,getXMLNodeFunc,doc,append_to_id)
{
    this.mgr = mgr;
    this.xsl_id = xsl_id;
    this.getXMLNodeFunc = getXMLNodeFunc;
    this.doc = doc;
    this.append_to_id = append_to_id;
}

function XSLMgrParam(name, value) {
    this.name = name;
    this.value = value;
}

if (window.ActiveXObject) {
    XSLManager.prototype.reset = function(xsl) {
        return true;
    }
    XSLManager.prototype.importStylesheet = function(xsl, xsldoc, xsl_id) {
        var xslTemplate = new ActiveXObject("MSXML2.XSLTemplate");
        xslTemplate.stylesheet = xsldoc;
        this.xsl = this.tximpstyles[xsl_id];
        if ((undefined == this.xsl) || (null == this.xsl)) {
            window.status = "Creating xsl processor...";
            this.xsl = xslTemplate.createProcessor();
            this.tximpstyles[xsl_id] = this.xsl;
        } else {
            window.status = "XSL processor recycled...";
        }
        //this.xsl = xslTemplate;
    }
    XSLManager.prototype.transformToFragment = function(xmlNode, doc) {
        var fragElmt = doc.createDocumentFragment();
        //fragElmt.innerHTML = xmlNode.transformNode(this.xsl);

        this.xsl.input = xmlNode;
        window.status = "Transforming XML Node: " + xmlNode.tagName;
        this.xsl.transform();
        fragElmt.innerHTML = this.xsl.output;

        window.status = "XML transformed ";

        return fragElmt;
    }
} else {
    XSLManager.prototype = new XSLTProcessor();
}

XSLManager.prototype.params = [];
XSLManager.prototype.intervalId = 0;
XSLManager.prototype.xmldoc = null;
XSLManager.prototype.txstyles = [];
XSLManager.prototype.tximpstyles = [];
XSLManager.prototype.addMgrParameter = function(paramName, paramValue) {
        this.params.push(new XSLMgrParam(paramName, paramValue));
};
XSLManager.prototype.addStylesheet = function(id,uri) {
    if (this.txstyles[id]==undefined) {
	this.txstyles[id] = new XSLDocumentManager(id, uri, LoadXSLOnManager, this);
    }
};
XSLManager.prototype.setXmXML = function(xml_uri) {
    if (arguments.length > 0) {
	this.location = xml_uri;
    } else {
	xml_uri = this.location;
    }
    this.xmldoc = new DocumentManager(xml_uri, LoadXMLOnManager, this);
    var resp = this.xmldoc.getResponse();
    this.xmldoc = resp;
};

/**  Transforms the XML content contained in the XSLManager into the document
 *   element with and id corresponding to append_to_id in the DOM specified by
 *   the doc parameter.
 *   @param xsl_id a string matching the label passed when a URI is registered
 *                 with the XSLManager via addStylesheet
 *   @param getXMLNodeFunc a function that returnes the desired node for 
 *                         transformation when passed the XML DOM
 *   @param doc a DOM instance in which the transformed results are to be 
 *              written
 *   @param append_to_id a string correlating to the id of the element in doc
 *                       to which the resultant DOM fragment will be appended
 **/
XSLManager.prototype.transform = function(xsl_id,getXMLNodeFunc,doc,append_to_id) {
    var selStyle = this.txstyles[xsl_id];

    if ((null == this.xmldoc) || (undefined == this.xmldoc.getElementById) ||
	(undefined == selStyle) || (null == selStyle) ) {
        if (null != this.xmldoc) window.status = "Transform failed, no results";
        else window.status = "Transform failed, no style templates";
        return false;
    }
    var txNode = this.xmldoc;
    if ((null != txNode) && (null != getXMLNodeFunc)) {
        txNode = getXMLNodeFunc(txNode);
    }
    if (null != txNode) {
        this.xsl = selStyle.getResponse();
        this.reset();
        this.importStylesheet(this.xsl, selStyle.reqObj, xsl_id);
        while (this.params.length > 0) {
            var po = this.params.pop();
            try {
                this.xsl.addParameter(po.name, po.value);
            } catch (e1) {
                try {
                    this.setParameter(null, po.name, po.value);
                } catch (e2) {
                    alert("Param passing through xsl not supported");
                }
            }
        }
        window.status = "Transforming result from target: " + append_to_id;
        var fragment = this.transformToFragment(txNode, document);
        var elmt = document.getElementById(append_to_id);
        if (null != elmt) {
            if (undefined == fragment.innerHTML) {
                while (elmt.hasChildNodes()) {
                    elmt.removeChild(elmt.lastChild);
                }
                elmt.appendChild(fragment);
            } else {
                elmt.innerHTML = fragment.innerHTML;
            }
            window.status = "Done";
        } else {
            window.status = "Failed: transformation element " + append_to_id +
                " not found on page";
        }
    }
    return true;
};

function XSLManager(xml_uri)
{
    this.setXmXML(xml_uri);
    this.intervalId = 0;
}

