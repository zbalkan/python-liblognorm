#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <string.h>
#include <liblognorm.h>
#include <errno.h>

#define MODULE_NAME "liblognorm"
#define TYPE_NAME   "Lognorm"

#define MODULE_DOCSTRING "Log normalization library."
#define TYPE_DOCSTRING   "liblognorm context"

// Exception types
static PyObject *LognormError;          // Base exception
static PyObject *LognormMemoryError;
static PyObject *LognormConfigError;
static PyObject *LognormParserError;
static PyObject *LognormRuleError;

static PyObject* liblognorm_version(PyObject *self, PyObject *args)
{
  return Py_BuildValue("s", ln_version());
}

typedef struct {
    PyObject_HEAD
    ln_ctx lognorm_context;
    char last_error[512];
} ObjectInstance;

static void py_err_callback(void *cookie, const char *msg, size_t lenMsg)
{
    ObjectInstance *self = (ObjectInstance *)cookie;
    size_t len = lenMsg < sizeof(self->last_error)-1 ? lenMsg : sizeof(self->last_error)-1;
    memcpy(self->last_error, msg, len);
    self->last_error[len] = '\0';
}

static
int obj_init(ObjectInstance *self, PyObject *args, PyObject *kwargs)
{
  char *rulebase;
  static char *kwlist[] = {"rules", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &rulebase))
    return -1;

  self->lognorm_context = ln_initCtx();
  if (self->lognorm_context == NULL) {
    PyErr_SetString(LognormMemoryError, "Failed to initialize liblognorm context");
    return -1;
  }
  self->last_error[0] = '\0';
  ln_setErrMsgCB(self->lognorm_context, py_err_callback, self);
  int result = ln_loadSamples(self->lognorm_context, rulebase);
  if (result != 0) {
    switch (result) {
      case LN_NOMEM:
        PyErr_NoMemory();
        break;
      case LN_BADCONFIG:
        PyErr_SetString(PyExc_ValueError, "bad configuration file");
        break;
      case LN_BADPARSERSTATE:
        PyErr_SetString(PyExc_RuntimeError, "bad parser state");
        break;
      case LN_WRONGPARSER:
        PyErr_SetString(PyExc_RuntimeError, "wrong parser");
        break;
      default:
        PyErr_SetFromErrno(PyExc_OSError);
        break;
    }
    return -1;
  }

  return 0;
}

static
void obj_dealloc(ObjectInstance *self)
{
  if (self->lognorm_context != NULL)
    ln_exitCtx(self->lognorm_context);
    memset(self->last_error, 0, sizeof(self->last_error));
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject* convert_object(json_object *obj);

static PyObject* liblognorm_get_error(ObjectInstance *self, PyObject *Py_UNUSED(ignored))
{
    if (self->last_error[0] == '\0')
        Py_RETURN_NONE;
    return Py_BuildValue("s", self->last_error);
}

// result = lognorm.normalize(log = "...", strip = True)
static
PyObject* normalize(ObjectInstance *self, PyObject *args, PyObject *kwargs)
{
  char *log_entry;
  Py_ssize_t log_entry_length;
  PyObject *strip = NULL;

  static char *kwlist[] = {"log", "strip", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|O", kwlist,
                                   &log_entry, &log_entry_length, &strip))
    return NULL;

  if (log_entry_length == 0) {
    Py_INCREF(Py_None);
    return Py_None;
  }

  if (strip != NULL && PyObject_IsTrue(strip)) {
    while (log_entry_length > 0 &&
           (log_entry[log_entry_length - 1] == '\n' ||
            log_entry[log_entry_length - 1] == '\r' ||
            log_entry[log_entry_length - 1] == '\t' ||
            log_entry[log_entry_length - 1] == ' '))
      log_entry_length--;
  }

  self->last_error[0] = '\0';
  struct json_object *log = NULL;
  int norm_result = ln_normalize(self->lognorm_context, log_entry,
                                 (size_t)log_entry_length, &log);

  if (norm_result != 0 || log == NULL) {
    switch (norm_result) {
        case LN_NOMEM:
            PyErr_SetString(LognormMemoryError, "Out of memory");
            return NULL;
        case LN_BADCONFIG:
            PyErr_SetString(LognormConfigError, "Invalid rulebase configuration");
            return NULL;
        case LN_BADPARSERSTATE:
            PyErr_SetString(LognormParserError, "Invalid parser state");
            return NULL;
        case LN_WRONGPARSER:
            PyErr_SetString(LognormParserError, "No matching parser or invalid message");
            return NULL;
        case LN_RB_LINE_TOO_LONG:
        case LN_OVER_SIZE_LIMIT:
            PyErr_SetString(LognormRuleError, "Rulebase line too long or over size limit");
            return NULL;
        default:
            if (self->last_error[0] != '\0')
                PyErr_SetString(LognormError, self->last_error);
            else
                PyErr_SetString(LognormError, "Unknown normalization error");
            return NULL;
      }
  }


  PyObject *result = convert_object(log);

  /* NOTE:
   * In liblognorm >= 2.x, ln_normalize() may free or reuse
   * the JSON internally; calling json_object_put(log) can segfault.
   * So we remove the unconditional free to avoid double-free.
   */
  // json_object_put(log);

  return result;
}

//----------------------------------------------------------------------------
// data conversion: json-c/libfastjson -> Python
//----------------------------------------------------------------------------

static PyObject* convert_scalar(json_object *obj);
static PyObject* convert_list(json_object *obj);
static PyObject* convert_hash(json_object *obj);

static
PyObject* convert_object(json_object *obj)
{
  if (obj == NULL) {
    Py_INCREF(Py_None);
    return Py_None;
  }

  switch (json_object_get_type(obj)) {
    case json_type_null:
    case json_type_boolean:
    case json_type_double:
    case json_type_int:
    case json_type_string:
      return convert_scalar(obj);
    case json_type_object:
      return convert_hash(obj);
    case json_type_array:
      return convert_list(obj);
    default:
      Py_INCREF(Py_None);
      return Py_None;
  }
}

static
PyObject* convert_scalar(json_object *obj)
{
  if (obj == NULL) {
    Py_INCREF(Py_None);
    return Py_None;
  }

  switch (json_object_get_type(obj)) {
    case json_type_null:
      Py_INCREF(Py_None);
      return Py_None;
    case json_type_boolean:
      if (json_object_get_boolean(obj)) {
        Py_INCREF(Py_True);
        return Py_True;
      } else {
        Py_INCREF(Py_False);
        return Py_False;
      }
    case json_type_double:
      return Py_BuildValue("d", json_object_get_double(obj));
    case json_type_int:
      return Py_BuildValue("l", json_object_get_int64(obj));
    case json_type_string:
      return Py_BuildValue("s", json_object_get_string(obj));
    default:
      Py_INCREF(Py_None);
      return Py_None;
  }
}

static
PyObject* convert_list(json_object *obj)
{
  if (obj == NULL) {
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyObject *result = PyList_New(0);
  int array_length = json_object_array_length(obj);
  for (int i = 0; i < array_length; ++i) {
    PyObject *item = convert_object(json_object_array_get_idx(obj, i));
    PyList_Append(result, item);
    Py_DECREF(item);
  }
  return result;
}

static
PyObject* convert_hash(json_object *obj)
{
  if (obj == NULL) {
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyObject *result = PyDict_New();
  struct json_object_iterator it = json_object_iter_begin(obj);
  struct json_object_iterator itEnd = json_object_iter_end(obj);

  while (!json_object_iter_equal(&it, &itEnd)) {
    PyObject *value = convert_object(json_object_iter_peek_value(&it));
    PyDict_SetItemString(result, json_object_iter_peek_name(&it), value);
    Py_DECREF(value);
    json_object_iter_next(&it);
  }

  return result;
}

//----------------------------------------------------------------------------
// Python module administrative stuff
//----------------------------------------------------------------------------

static PyMethodDef object_methods[] = {
  {"normalize", (PyCFunction)normalize, METH_VARARGS | METH_KEYWORDS,
    "parse log line to dict object"},
  {"version", (PyCFunction)liblognorm_version, METH_VARARGS,
    "return liblognorm's version"},
  {"get_error", (PyCFunction)liblognorm_get_error, METH_NOARGS,
    "return last error message from liblognorm"},
  {NULL}
};

static PyTypeObject TypeObject = {
    PyVarObject_HEAD_INIT(NULL, 0)
    MODULE_NAME "." TYPE_NAME,           /* tp_name */
    sizeof(ObjectInstance),              /* tp_basicsize */
    0,                                   /* tp_itemsize */
    (destructor)obj_dealloc,             /* tp_dealloc */
    0,                                   /* tp_vectorcall_offset */
    0,                                   /* tp_getattr */
    0,                                   /* tp_setattr */
    0,                                   /* tp_as_async */
    0,                                   /* tp_repr */
    0,                                   /* tp_as_number */
    0,                                   /* tp_as_sequence */
    0,                                   /* tp_as_mapping */
    0,                                   /* tp_hash  */
    0,                                   /* tp_call */
    0,                                   /* tp_str */
    0,                                   /* tp_getattro */
    0,                                   /* tp_setattro */
    0,                                   /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                  /* tp_flags */
    TYPE_DOCSTRING,                      /* tp_doc */
    0,                                   /* tp_traverse */
    0,                                   /* tp_clear */
    0,                                   /* tp_richcompare */
    0,                                   /* tp_weaklistoffset */
    0,                                   /* tp_iter */
    0,                                   /* tp_iternext */
    object_methods,                      /* tp_methods */
    0,                                   /* tp_members */
    0,                                   /* tp_getset */
    0,                                   /* tp_base */
    0,                                   /* tp_dict */
    0,                                   /* tp_descr_get */
    0,                                   /* tp_descr_set */
    0,                                   /* tp_dictoffset */
    (initproc)obj_init,                  /* tp_init */
    0,                                   /* tp_alloc */
    PyType_GenericNew,                   /* tp_new */
};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    MODULE_NAME,         /* m_name */
    MODULE_DOCSTRING,    /* m_doc */
    -1,                  /* m_size */
    object_methods,      /* m_methods */
    NULL,                /* m_reload */
    NULL,                /* m_traverse */
    NULL,                /* m_clear */
    NULL,                /* m_free */
};

PyMODINIT_FUNC PyInit_liblognorm(void)
{
  PyObject* module;
  TypeObject.tp_new = PyType_GenericNew;
  if (PyType_Ready(&TypeObject) < 0)
    return NULL;


  module = PyModule_Create(&moduledef);

  // Create exception hierarchy
  LognormError = PyErr_NewException("liblognorm.Error", NULL, NULL);
  Py_INCREF(LognormError);
  PyModule_AddObject(module, "Error", LognormError);

  LognormMemoryError = PyErr_NewException("liblognorm.MemoryError", LognormError, NULL);
  Py_INCREF(LognormMemoryError);
  PyModule_AddObject(module, "MemoryError", LognormMemoryError);

  LognormConfigError = PyErr_NewException("liblognorm.ConfigError", LognormError, NULL);
  Py_INCREF(LognormConfigError);
  PyModule_AddObject(module, "ConfigError", LognormConfigError);

  LognormParserError = PyErr_NewException("liblognorm.ParserError", LognormError, NULL);
  Py_INCREF(LognormParserError);
  PyModule_AddObject(module, "ParserError", LognormParserError);

  LognormRuleError = PyErr_NewException("liblognorm.RuleError", LognormError, NULL);
  Py_INCREF(LognormRuleError);
  PyModule_AddObject(module, "RuleError", LognormRuleError);

  Py_INCREF(&TypeObject);
  PyModule_AddObject(module, TYPE_NAME, (PyObject *)&TypeObject);
  return module;
}
