//An N-dimensional curve fitting tool written in C Python
//GNU license applies to v0.3 including v0.3.x and later versions
//Copyright (C) 2014	Michael Winters : micwinte@chalmers.se

//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA	02110-1301, USA.

// Python includes
#include <Python.h>
#include <structmember.h>

// Preprocessor define PyMODINIT
#ifndef PyMODINIT_FUNC	
#define PyMODINIT_FUNC void
#endif


//////////////////////////////////////////////
// General Structure for PyObject Definition
//
// 1) typedef
// 2) destructor
// 3) constructor for empty object __new__
// 4) constructor for object __init__
// 5) class methods 
// 6) member & method definitions for new type (class)
// 7) build class by calling PyTypeObject
// 8) module methods
// 9) method definition for module
// 0) initialization
//

// Python exception object 
static PyObject* ndfitError;

// 1) typedef: data structure definition
typedef struct ndfit{

	PyObject_HEAD

	// Data members
	PyObject* data;
	PyObject* pList;
	PyObject* consts;
	PyObject* fitfunc;
	PyObject* errfunc;
	PyObject* lattice;	

} ndFit;

// 2) typedef destructor
void ndFit_dealloc(ndFit* self){

	// use XDECREF because first and last could be NULL
	Py_XDECREF(self->data);
	Py_XDECREF(self->pList);
	Py_XDECREF(self->consts);
	Py_XDECREF(self->fitfunc); 
	Py_XDECREF(self->errfunc);
	Py_XDECREF(self->lattice);

	// actually free the memory by calling tp_free
	//self->ob_type->tp_free((PyObject*)self);
}

/////////////////////////////////
// CONSTRUCTOR: everything empty 
// 3a) Constructor for empty object __new__ 
// 		PyTypeObject-->tp_new
PyObject* ndFit_new(PyTypeObject* type, PyObject* args, PyObject* kwds){
 
	ndFit *self; 
	Py_ssize_t empty = 0;
	self = (ndFit*)type->tp_alloc(type,0);
	if(self!=NULL){
		
		// Create empty lists and none objects for the functions
		self->data		= PyList_New(empty);
		self->pList	 = PyList_New(empty);
		self->consts	= PyList_New(empty);
		self->fitfunc = Py_None;						
		self->errfunc = Py_None;
		self->lattice = Py_None;

		if (self->data == NULL){Py_DECREF(self);return NULL;}
		if (self->pList == NULL){Py_DECREF(self);return NULL;}
		if (self->consts == NULL){Py_DECREF(self);return NULL;}
		if (self->fitfunc == NULL){Py_DECREF(self);return NULL;}
		if (self->errfunc == NULL){Py_DECREF(self);return NULL;}
		if (self->lattice == NULL){Py_DECREF(self);return NULL;}
	}
	return (PyObject*)self;
	//return 0;
}
////////////////////////////////////
// CONSTRUCTOR: initialize values //
////////////////////////////////////
// set this is PyTypeObject-->tp_init
static int
ndFit_init(ndFit* self, PyObject* args){

	PyObject* data	= NULL;
	PyObject* pList = NULL;
	PyObject* consts = NULL;
	PyObject* fitfunc = NULL; 
	PyObject* errfunc = NULL;
	PyObject* lattice = NULL;

	PyObject* tmp;
	if (!PyArg_ParseTuple(args,"OOOOOO",&data, &pList,&consts, &fitfunc, &errfunc, &lattice)){return -1;}

	if (data) {tmp=self->data; Py_INCREF(data); self->data = data; Py_XDECREF(tmp);}
	if (data) {tmp=self->pList; Py_INCREF(pList); self->pList = pList; Py_XDECREF(tmp);}
	if (data) {tmp=self->consts; Py_INCREF(consts); self->consts = consts; Py_XDECREF(tmp);}
	if (data) {tmp=self->fitfunc; Py_INCREF(fitfunc); self->fitfunc = fitfunc; Py_XDECREF(tmp);}
	if (data) {tmp=self->errfunc; Py_INCREF(errfunc); self->errfunc = errfunc; Py_XDECREF(tmp);}
	if (data) {tmp=self->lattice; Py_INCREF(lattice); self->lattice = lattice; Py_XDECREF(tmp);}
	return 0;
}

///////////////////////////////////////////////
// CONSTRUCTOR: MEMBER DEFINITION FOR PYTHON //
///////////////////////////////////////////////
// set this is PyTypeObject-->tp_members
PyMemberDef ndFit_members[] = {
	{"data",T_OBJECT_EX,offsetof(ndFit,data),0,"input data"},
	{"pList",T_OBJECT_EX,offsetof(ndFit,pList),0,"parameter list [entropy,(params)]"},
	{"consts",T_OBJECT_EX,offsetof(ndFit,consts),0,"constant parameter list [consts]]"},
	{"fitfunc",T_OBJECT_EX,offsetof(ndFit,fitfunc),0,"fit function used"},
	{"errfunc",T_OBJECT_EX,offsetof(ndFit,errfunc),0,"error function used"},
	{"lattice",T_OBJECT_EX,offsetof(ndFit,lattice),0,"fit lattice for error checking"},
	{NULL}	 /* Sentinel */
};

/////////////////////////////////////////////////////////
// ~~~~~~~~~~~~~~~ METHOD DEFINITION ~~~~~~~~~~~~~~~~~~//
/////////////////////////////////////////////////////////

// Method to get the immediate result
static PyObject* ndFit_getresult(ndFit* self){
	
	PyObject* result; 
	Py_ssize_t sizep = PyList_Size(self->pList);
	result = PyList_GetItem(self->pList,sizep-1);
	Py_INCREF(result);
	return result;
}

// Method to get all of the entropy values
static PyObject* ndFit_getentropy(ndFit* self){

	PyObject* list;
	Py_ssize_t iter; 
	Py_ssize_t size = PyList_Size(self->pList);
	list = PyList_New(size);
	Py_INCREF(list);

	PyObject* tmp; 
	for(iter = 0; iter<size; iter+=1){ 
		tmp = PyList_GetItem(self->pList,iter);
		PyList_SetItem(list,iter,PyTuple_GetItem(tmp,0));
		Py_DECREF(tmp);
	}
	return list;
}

static PyObject* ndFit_buildcurve(ndFit* self, PyObject* args, PyObject* kwds){
	
	// Import the values
	PyObject* values;
	if(!PyArg_ParseTuple(args,"O",&values)){return NULL;}

	PyObject* result; 
	PyObject* params;
	Py_ssize_t sizep = PyList_Size(self->pList);
	result = PyList_GetItem(self->pList,sizep-1);
	params = PyTuple_GetItem(result,1);
	Py_INCREF(result);
	Py_INCREF(params);

	if(!PyList_Check(values)){
		PyErr_SetString(ndfitError,"Data is not a list: please remember to zip your lists");
		return NULL;
	}

	Py_ssize_t size = PyList_Size(values);
	PyObject *curve = PyList_New(size);
 
	int i = 0; 
	double v;
	Py_ssize_t len = 1;
	Py_ssize_t pos = 0;
	PyObject* tmp	= PyList_New(len);
 
	PyObject* pt;

	Py_INCREF(tmp);
	for (i=0;i<size;i+=1){
		v =	PyFloat_AsDouble( PyList_GetItem(values,i));
		PyList_SetItem(tmp, pos ,Py_BuildValue("f",v));
		pt = PyObject_CallFunctionObjArgs(self->fitfunc,tmp,params,self->consts,NULL);
		PyList_SetItem(curve,i,pt);
	}
	return curve;
}

///////////////////////
// METHOD DEFINITION //
///////////////////////
PyMethodDef ndFit_methods[] = {
	{"getresult" , (PyCFunction)(void(*)(void))ndFit_getresult, METH_NOARGS, "return the final result"},
	{"getentropy", (PyCFunction)(void(*)(void))ndFit_getentropy, METH_NOARGS,"return a list of the entropy values"},
	{"buildcurve", (PyCFunction)(void(*)(void))ndFit_buildcurve, METH_VARARGS|METH_KEYWORDS, "build the optimized curve"},
	{NULL}	/* Sentinel */
}; 

 
/////////////////////////////////////////////////////////
// ~~~~~~~~~~~~~~~~~ BUILD OBJECT ~~~~~~~~~~~~~~~~~~~~ //
/////////////////////////////////////////////////////////

///////////////////////////
// NEW PYOBJECT: TYPEDEF //
///////////////////////////
// everything listed below needs to show up above
PyTypeObject ndFitType = {
		PyObject_HEAD_INIT(NULL)
		"ndfit.ndFit",									 /* tp_name */
		sizeof(ndFit),									 /* tp_basicsize */
		0,												 /* tp_itemsize */
		(destructor)ndFit_dealloc, 						 /* tp_dealloc */
	    0,												 /* tp_print */
	    0,												 /* tp_getattr */
	    0,												 /* tp_setattr */
	    0,												 /* tp_compare */
	    0,												 /* tp_repr */
	    0,												 /* tp_as_number */
	    0,												 /* tp_as_sequence */
	    0,												 /* tp_as_mapping */
	    0,												 /* tp_hash */
	    0,												 /* tp_call */
	    0,												 /* tp_str */
	    0,												 /* tp_getattro */
	    0,												 /* tp_setattro */
	    0,												 /* tp_as_buffer */
	    Py_TPFLAGS_DEFAULT,								 /* tp_flags */
	    0,												 /* tp_doc */
		0,												 /* tp_traverse */
		0,												 /* tp_clear */
		0,												 /* tp_richcompare */
		0,												 /* tp_weaklistoffset */
		0,												 /* tp_iter */
		0,												 /* tp_iternext */
		ndFit_methods,									 /* tp_methods */
		ndFit_members,									 /* tp_members */
		0,												 /* tp_getset */
		0,												 /* tp_base */
		0,												 /* tp_dict */
		0,												 /* tp_descr_get */
		0,												 /* tp_descr_set */
		0,												 /* tp_dictoffset */
		(initproc)ndFit_init,							 /* tp_init */
		0,												 /* tp_alloc */
		ndFit_new,										 /* tp_new */
};
