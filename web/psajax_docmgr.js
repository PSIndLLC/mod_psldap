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
    window.status = "using activex controls";
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
    window.status = "using document implementation";
    if (typeof XMLHttpRequest != 'undefined') {
        this.reqObj = new XMLHttpRequest();
    } else {
        this.reqObj = new IFrameRequest();
    }
};

RequestWrapper.prototype.init = function()
{
    if (window.ActiveXObject) {
        this.initActiveX();
    } else if (document.implementation && document.implementation.createDocument) {
        this.initDOMImpl();
    } else {
        this.reqObj = new DummyDOM();
    }
    this.getResponse = function() {
	return ((undefined != this.reqObj.responseText) ? this.reqObj.responseText :
		((undefined != this.reqObj.responseXML) ? this.reqObj.responseXML : 
		 this.reqObj.documentElement.xml) );
    }
};

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
        try {
            this.reqObj.async = false;
            this.reqObj.validateOnParse = false;
            // For some reason this is failing with the freethreadeddomdocument...
	    if ((undefined == this.reqObj.load) ||
		(!this.reqObj.load(docref))){ 
		throw("load undefined on Request Object");
	    } else {
                window.status = "docref loaded " + docref;
	    }
        } catch(e1) {
            window.status = "Exception raise loading docref: " + e1.message;
            this.reqObj.open("GET", docref, false);
            this.reqObj.send(null);
        }
        onload.call(this, null);
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

        window.status = "Frag Text is: " + fragElmt.innerHTML;

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
        this.xmldoc = new DocumentManager(xml_uri, LoadXMLOnManager, this);
        var resp = this.xmldoc.getResponse();
        this.xmldoc = resp;
};

XSLManager.prototype.transform = function(xsl_id,getXMLNodeFunc,doc,append_to_id) {
    var selStyle = this.txstyles[xsl_id];
    window.status = "Transforming " + this.xmldoc + " with " + xsl_id + ":" + selStyle;
    if ((null == this.xmldoc) || (undefined == selStyle) ||
        (null == selStyle) ) {
        if (null != this.xmldoc) window.status = "Transform failed...";
        else window.status = "";
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
        window.status = "Transforming fragment... target = " + append_to_id;
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
            window.status = "elmt html is now : " + elmt.innerHTML;
        } else {
            window.status = "Transformation element " + append_to_id +
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

