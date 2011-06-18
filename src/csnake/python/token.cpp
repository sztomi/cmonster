/*
Copyright (c) 2011 Andrew Wilkins <axwalk@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* Define this to ensure only the limited API is used, so we can ensure forward
 * binary compatibility. */
#define Py_LIMITED_API

#include <Python.h>
#include <stdexcept>

#include "token.hpp"
#include <iostream>

namespace csnake {
namespace python {

static PyTypeObject *TokenType = NULL;
PyDoc_STRVAR(Token_doc, "Token objects");

struct Token
{
    PyObject_HEAD
    csnake::core::token_type *token;
};

static void Token_dealloc(Token* self)
{
    typedef csnake::core::token_type token_type;
    if (self->token)
        delete self->token;
    PyObject_Del((PyObject*)self);
}

Token* create_token(csnake::core::token_type const& token_value)
{
    Token *token = (Token*)PyObject_CallObject((PyObject*)TokenType, NULL);
    if (token)
        token->token = new csnake::core::token_type(token_value);
    return token;
}

static int Token_init(Token *self, PyObject *args, PyObject *kwargs)
{
    static const char *keywords[] = {"id", "value", NULL};
    int token_id = 0;
    PyObject *value = NULL;

    if (!PyArg_ParseTupleAndKeywords(
             args, kwargs, "|iO", (char**)keywords, &token_id, &value))
        return -1; 

    // Convert the "value" object to a string.
    csnake::core::token_type::string_type value_str;
    if (value)
    {
        PyObject *strobj = PyObject_Str(value);
        if (strobj)
        {
            PyObject *utf8 = PyUnicode_AsUTF8String(strobj);
            if (utf8)
            {
                const char *value_chars = PyBytes_AsString(utf8);
                if (value_chars)
                    value_str = value_chars;
                Py_DECREF(utf8);
            }
            Py_DECREF(strobj);
        }
    }

    // Create the boost::wave token.
    self->token = new csnake::core::token_type(
        static_cast<boost::wave::token_id>(token_id),
        value_str,
        csnake::core::token_type::position_type("?")
    );

    return 0;
}

static PyObject* Token_str(Token *self)
{
    csnake::core::token_type::string_type const& value =
        self->token->get_value();
    return PyUnicode_FromStringAndSize(value.data(), value.size());
}

static PyObject* Token_repr(Token *self)
{
    BOOST_WAVE_STRINGTYPE const& name =
        boost::wave::get_token_name(*self->token);
    csnake::core::token_type::string_type const& value =
        self->token->get_value();
    csnake::core::token_type::position_type const& position =
        self->token->get_position();
    return PyUnicode_FromFormat("Token(T_%s, '%s', %s:%u:%u)",
                                name.c_str(), value.c_str(),
                                position.get_file().c_str(),
                                position.get_line(), position.get_column());
}

static PyObject* Token_get_token_id(Token *self, void *closure)
{
    return PyLong_FromLong(static_cast<boost::wave::token_id>(*self->token));
}

static PyObject*
Token_set_token_id(Token *self, PyObject *value, void *closure)
{
    long id = PyLong_AsLong(value);
    if (id == -1 && PyErr_Occurred())
        return NULL;
    self->token->set_token_id(static_cast<boost::wave::token_id>(id));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyGetSetDef Token_getset[] = {
    {"token_id", (getter)Token_get_token_id, (setter)Token_set_token_id,
     NULL /* docs */, NULL /* closure */},
    {NULL}
};

static PyType_Slot TokenTypeSlots[] =
{
    {Py_tp_dealloc, (void*)Token_dealloc},
    {Py_tp_getset,  (void*)Token_getset},
    {Py_tp_str,     (void*)Token_str},
    {Py_tp_repr,    (void*)Token_repr},
    {Py_tp_doc,     (void*)Token_doc},
    {Py_tp_init,    (void*)Token_init},
    {0, NULL}
};

static PyType_Spec TokenTypeSpec =
{
    "csnake._preprocessor.Token",
    sizeof(Token),
    0,
    Py_TPFLAGS_DEFAULT,
    TokenTypeSlots
};

PyTypeObject* init_token_type()
{
    TokenType = (PyTypeObject*)PyType_FromSpec(&TokenTypeSpec);
    if (!TokenType)
        return NULL;
    if (PyType_Ready(TokenType) < 0)
        return NULL;
    return TokenType;
}

PyTypeObject* get_token_type()
{
    return TokenType;
}

csnake::core::token_type get_token(Token *wrapper)
{
    return wrapper ? *wrapper->token : csnake::core::token_type();
}

}}

