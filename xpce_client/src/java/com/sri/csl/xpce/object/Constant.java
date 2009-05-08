package com.sri.csl.xpce.object;

import org.json.JSONException;
import org.json.JSONObject;

import com.sri.csl.xpce.json.XPCEConstants;

public class Constant extends Term {
	protected Object name;
	
	public Constant(Object name) {
		this.name = name;
	}

	public Constant(JSONObject obj) throws JSONException {
		name = obj.get(XPCEConstants.CONST);
	}

	public JSONObject toJSON() throws JSONException {
		JSONObject obj = new JSONObject();
		obj.put(XPCEConstants.CONST, name);
		return obj;
	}
	
	public String toString() {
		return name.toString();
	}
}
