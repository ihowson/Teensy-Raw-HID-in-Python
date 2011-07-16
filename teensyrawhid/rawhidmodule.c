/*
 * Copyright (c) 2011 Ian Howson (ian@mutexlabs.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <Python.h>
#include "structmember.h"
#include "hid.h"

typedef struct {
    PyObject_HEAD
    int opened;
} Rawhid;

static void Rawhid_dealloc(Rawhid* self)
{
    // TODO want to make sure device is closed in here
    self->ob_type->tp_free((PyObject*)self);
}

static PyMethodDef module_methods[] = {
    {NULL}
};

static PyObject* Rawhid_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Rawhid *self;

    self = (Rawhid *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->opened = 0;
    }

    return (PyObject *)self;
}

static int Rawhid_init(Rawhid *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static PyMemberDef Rawhid_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject * Rawhid_open(Rawhid* self, PyObject *args, PyObject *keywords)
{
    // close automatically if already opened
    if (self->opened)
    {
        if (!PyObject_CallMethod((PyObject *)self, "close", NULL))
        {
            return NULL;
        }
    }

    /* We don't expose the 'maximum number of devices' parameter to rawhid; it
     * complicates the Python interface and I couldn't think of a use for it.
     * If you want it, send me an email. */
    static char *kwlist[] = {"vid", "pid", "usage_page", "usage", NULL};

    int vid = 0x16c0;
    int pid = 0x0480;
    int usage_page = 0xffab;
    int usage = 0x0200;

    int ok = PyArg_ParseTupleAndKeywords(args, keywords, "|iiii", kwlist, &vid, &pid, &usage_page, &usage);
    if (!ok)
    {
        return NULL;
    }

    //printf("vid = 0x%x, pid = 0x%x, up = 0x%x, u = 0x%x\n", vid, pid, usage_page, usage);

    int dev = rawhid_open(1, vid, pid, usage_page, usage);

    if (dev)
    {
        // found a device
        self->opened = 1;
        Py_RETURN_NONE;
    }
    else
    {
        PyErr_SetString(PyExc_IOError, "No device found");
        return NULL;
    }
}

static PyObject * Rawhid_send(Rawhid* self, PyObject *args, PyObject *kwds)
{
    uint8_t *buf;
    int size;
    int timeout; // TODO: use default timeout value from constructor

    if (!self->opened)
    {
        PyErr_SetString(PyExc_IOError, "Device is not open");
        return NULL;
    }

    // extract the frame from args
    int ok = PyArg_ParseTuple(args, "s#i", &buf, &size, &timeout);
    if (!ok)
        return NULL;

    int ret = rawhid_send(0, buf, size, timeout);
    if (ret == size)
    {
        Py_RETURN_NONE; // ok
    }
    else if (ret < 0)
    {
        // other error
        return PyErr_SetFromErrno(PyExc_IOError);
    }
    else
    {
        return PyErr_Format(PyExc_IOError, "Sent less data than requested (sent %d bytes, expected %d bytes)", ret, size);
    }
}

static PyObject * Rawhid_recv(Rawhid* self, PyObject *args, PyObject *kwds)
{
    uint8_t *buf;
    int size;
    int timeout; // TODO: use default timeout

    if (!self->opened)
    {
        PyErr_SetString(PyExc_IOError, "Device is not open");
        return NULL;
    }

    int ok = PyArg_ParseTuple(args, "ii", &size, &timeout);
    if (!ok)
        return NULL;
    
    buf = calloc(size, sizeof(uint8_t));
    if (!buf)
    {
        return PyErr_NoMemory();
    }

    int ret = rawhid_recv(0, buf, size, timeout);
    if (ret == size)
    {
        PyObject *ret = Py_BuildValue("s#", buf, size);
        free(buf);
        return ret;
    }
    else if (ret < 0)
    {
        // other error
        free(buf);
        return PyErr_SetFromErrno(PyExc_IOError);
    }
    else
    {
        free(buf);
        return PyErr_Format(PyExc_IOError, "Received less data than requested (got %d bytes, expected %d bytes)", ret, size);
    }

    // I assume that buf is copied, per http://docs.python.org/release/2.0/ext/buildValue.html
    return Py_BuildValue("s#", buf, ret);
}

// TODO want to make sure this is executed when we are GCed
static PyObject * Rawhid_close(Rawhid* self)
{
    if (self->opened)
    {
        rawhid_close(0);
        self->opened = 0;
    }

    Py_RETURN_NONE;
}

static PyObject * Rawhid_isOpened(Rawhid* self)
{
    if (self->opened)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}


// TODO 'default send timeout' parameter, used for rawhid_send if not specified there
// TODO 'default recv timeout' parameter, used for rawhid_recv if not specified there

// TODO: revise the help text to make more sense for python
// FIXME: make the descriptions match the style of other Python modules
static PyMethodDef Rawhid_methods[] = {
    {"open", (PyCFunction)Rawhid_open, METH_VARARGS | METH_KEYWORDS, 
        "Opens a single device that matches vid, pid, usage_page and usage. Defaults are suitable for a Teensy++ 2.0 device. \n\nIf you open() while the device is already opened, the original device will be closed first. You may get the same device on the open()."},
    {"close", (PyCFunction)Rawhid_close, METH_VARARGS | METH_KEYWORDS, 
        "Close device. It is safe to call this method even if the device is already closed."},
    {"send", (PyCFunction)Rawhid_send, METH_VARARGS | METH_KEYWORDS, 
        "Send a packet to the device. String 'buf' contains the data to transmit. Wait up to 'timeout' milliseconds. Return is the number of bytes sent, or zero if unable to send before timeout, or -1 if an error (typically indicating the device was unplugged)."},
    {"recv", (PyCFunction)Rawhid_recv, METH_VARARGS | METH_KEYWORDS, 
        "Receive a packet from device 'num' (zero based). Buffer 'buf' receives the data, and 'len' must be the packet size (default is 64 bytes). Wait up to 'timeout' milliseconds. Return is the number of bytes received, or zero if no packet received within timeout, or -1 if an error (typically indicating the device was unplugged)."},
    {"isOpened", (PyCFunction)Rawhid_isOpened, METH_NOARGS, "Returns True if the device is currently open, False otherwise."},
    {NULL}
};

static PyTypeObject RawhidType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "TeensyRawhid.Rawhid",           /*tp_name*/
    sizeof(Rawhid), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Rawhid_dealloc,                         /*tp_dealloc*/ // TODO might want one of these
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "",           /* tp_doc */
  0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Rawhid_methods,             /* tp_methods */
    Rawhid_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Rawhid_init,      /* tp_init */
    0,                         /* tp_alloc */
    Rawhid_new,                 /* tp_new */
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC initTeensyRawhid(void) 
{
    PyObject* m;

    RawhidType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&RawhidType) < 0)
        return;

    m = Py_InitModule3("TeensyRawhid", module_methods,
        "Allows you to use the USB Raw HID interface in Python.");

    Py_INCREF(&RawhidType);
    PyModule_AddObject(m, "Rawhid", (PyObject *)&RawhidType);
}

