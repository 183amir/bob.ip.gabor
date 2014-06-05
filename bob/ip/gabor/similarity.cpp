/**
 * @author Manuel Guenther <manuel.guenther@idiap.ch>
 * @date Wed Jun  4 17:44:24 CEST 2014
 *
 * @brief Bindings for a Gabor jet similarity
 *
 * Copyright (C) 2011-2014 Idiap Research Institute, Martigny, Switzerland
 */

#define BOB_IP_GABOR_MODULE
#include "main.h"
#include <bob.blitz/cppapi.h>
#include <bob.blitz/cleanup.h>
#include <bob.io.base/api.h>
#include <bob.extension/documentation.h>


static inline char* c(const char* o){return const_cast<char*>(o);}


/******************************************************************/
/************ Constructor Section *********************************/
/******************************************************************/

static auto Similarity_doc = bob::extension::ClassDoc(
  BOB_EXT_MODULE_PREFIX ".Similarity",
  "A class that computes different kind of similarity function, i.a., disparity corrected Gabor phase differences.",
  "The implementation is according to [Guenther2011]_ and [Guenther2012]_, where all similarity functions are explained in more details."
).add_constructor(
  bob::extension::FunctionDoc(
    "__init__",
    "Creates a Gabor wavelet similarity function of the given type",
    "Currently, several types of Gabor jet similarity functions are implemented. "
    "Please refer to [Guenther2012]_ for details.",
    true
  )
  .add_prototype("type, [transform]", "")
  .add_prototype("hdf5", "")
  .add_parameter("type", "str", "The type of the Gabor jet similarity function; might be one of ('ScalarProduct', 'Canberra', 'Disparity', 'PhaseDiff', 'PhaseDiffPlusCanberra')")
  .add_parameter("transform", ":py:class:`bob.ip.gabor.Transform`", "The Gabor wavelet transform class that was used to generate the Gabor jets; only required for disparity-based similarity functions ('Disparity', 'PhaseDiff', 'PhaseDiffPlusCanberra')")
  .add_parameter("hdf5", ":py:class:`bob.io.base.HD5Ffile`", "An HDF5 file open for reading to load the parametrization of the Gabor wavelet similarity from")
);

static int PyBobIpGaborJetSimilarity_init(PyBobIpGaborJetSimilarityObject* self, PyObject* args, PyObject* kwargs) {

  char* kwlist1[] = {c("hdf5"), NULL};
  char* kwlist2[] = {c("type"), c("transform"), NULL};

  // two ways to call
  PyObject* k = Py_BuildValue("s", kwlist1[0]);
  auto k_ = make_safe(k);
  if (
    (kwargs && PyDict_Contains(kwargs, k)) ||
    (args && PyTuple_Size(args) == 1 && PyBobIoHDF5File_Check(PyTuple_GetItem(args, 0)))
  ){
    PyBobIoHDF5FileObject* hdf5;
    if (
      !PyArg_ParseTupleAndKeywords(
        args, kwargs,
        "O&", kwlist1,
        &PyBobIoHDF5File_Converter, &hdf5
      )
    ){
      Similarity_doc.print_usage();
      return -1;
    }
    auto hdf5_ = make_safe(hdf5);

    self->cxx.reset(new bob::ip::gabor::Similarity(*hdf5->f));
  } else {
    const char* name = 0;
    PyBobIpGaborWaveletTransformObject* gwt = 0;
    if (
      !PyArg_ParseTupleAndKeywords(
        args, kwargs,
        "s|O!", kwlist2,
        &name,
        &PyBobIpGaborWaveletTransformType, &gwt
      )
    ){
      Similarity_doc.print_usage();
      return -1;
    }
    auto _ = make_xsafe(gwt);
//    Py_XINCREF(gwt);
    try{
      if (gwt)
        self->cxx.reset(new bob::ip::gabor::Similarity(bob::ip::gabor::Similarity::name_to_type(name), *gwt->cxx));
      else
        self->cxx.reset(new bob::ip::gabor::Similarity(bob::ip::gabor::Similarity::name_to_type(name)));
    }
    catch (std::exception& e) {
      PyErr_SetString(PyExc_RuntimeError, e.what());
      return -1;
    }
    catch (...) {
      PyErr_Format(PyExc_RuntimeError, "%s cannot create Gabor jet similarity: unknown exception caught", Py_TYPE(self)->tp_name);
      return -1;
    }
  }
  return 0;
}

static void PyBobIpGaborJetSimilarity_delete(PyBobIpGaborJetSimilarityObject* self) {
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int PyBobIpGaborJetSimilarity_Check(PyObject* o) {
  return PyObject_IsInstance(o, reinterpret_cast<PyObject*>(&PyBobIpGaborJetSimilarityType));
}


/******************************************************************/
/************ Variables Section ***********************************/
/******************************************************************/

static auto type_doc = bob::extension::VariableDoc(
  "type",
  "str",
  "The type of the Gabor jet similarity function"
);
PyObject* PyBobIpGaborJetSimilarity_type(PyBobIpGaborJetSimilarityObject* self, void*){
  return Py_BuildValue("s", self->cxx->type().c_str());
}

static auto lastDisparity_doc = bob::extension::VariableDoc(
  "last_disparity",
  "(float, float)",
  "The disparity that was computed during the last call to :py:func:`similarity` or :py:func:`disparity`."
);
PyObject* PyBobIpGaborJetSimilarity_lastDisparity(PyBobIpGaborJetSimilarityObject* self, void*){
  const auto& disp = self->cxx->disparity();
  return Py_BuildValue("(dd)", disp[0], disp[1]);
}


static PyGetSetDef PyBobIpGaborJetSimilarity_getseters[] = {
  {
    type_doc.name(),
    (getter)PyBobIpGaborJetSimilarity_type,
    0,
    type_doc.doc(),
    0
  },
  {
    lastDisparity_doc.name(),
    (getter)PyBobIpGaborJetSimilarity_lastDisparity,
    0,
    lastDisparity_doc.doc(),
    0
  },
  {0}  /* Sentinel */
};


/******************************************************************/
/************ Functions Section ***********************************/
/******************************************************************/

static auto similarity_doc = bob::extension::FunctionDoc(
  "similarity",
  "This function computes the similarity between the two given Gabor jets",
  "Depending on the :py:attr:`type`, different kinds of similarities are computed (see [Guenther2011]_ for details). "
  "Some of them will also compute the disparity from the first to the second Gabor jet, which can be retrieved by :py:attr:`last_disparity`.\n\n"
  ".. note:: The function :py:func:`__call__` is a synonym for this function.",
  true
)
.add_prototype("jet1, jet2", "sim")
.add_parameter("jet1, jet2", ":py:class:`bob.ip.gabor.Jet`", "The two Gabor jets that should be compared")
.add_return("sim", "float", "The similarity between the two Gabor jets; more similar Gabor jets will get higher similarity values")
;

static PyObject* PyBobIpGaborJetSimilarity_similarity(PyBobIpGaborJetSimilarityObject* self, PyObject* args, PyObject* kwargs) {

  static char* kwlist[] = {c("jet1"), c("jet2"), 0};

  PyBobIpGaborJetObject* jet1 = 0,* jet2 = 0;

  if (
    !PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!", kwlist,
      &PyBobIpGaborJetType, &jet1,
      &PyBobIpGaborJetType, &jet2
    )
  ){
    similarity_doc.print_usage();
    return 0;
  }

  auto j1_ = make_safe(jet1), j2_ = make_safe(jet2);
//  Py_INCREF(jet1);
//  Py_INCREF(jet2);

  try {
    double sim = self->cxx->similarity(*jet1->cxx, *jet2->cxx);
    return Py_BuildValue("d", sim);
  }
  catch (std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return 0;
  }
  catch (...) {
    PyErr_Format(PyExc_RuntimeError, "%s cannot compute Gabor jet similarity: unknown exception caught", Py_TYPE(self)->tp_name);
    return 0;
  }
}


static auto disparity_doc = bob::extension::FunctionDoc(
  "disparity",
  "This function computes the disparity vector for the given Gabor jets",
  "This function is only available for convenience, it **does not** need to be called before :py:func:`similarity` is called.",
  true
)
.add_prototype("jet1, jet2", "disparity")
.add_parameter("jet1, jet2", ":py:class:`bob.ip.gabor.Jet`", "The two Gabor jets to compute the disparity between")
.add_return("disparity", "(float, float)", "The disparity vector estimated from the given Gabor jets")
;

static PyObject* PyBobIpGaborJetSimilarity_disparity(PyBobIpGaborJetSimilarityObject* self, PyObject* args, PyObject* kwargs) {

  static char* kwlist[] = {c("jet1"), c("jet2"), 0};

  PyBobIpGaborJetObject* jet1 = 0,* jet2 = 0;;

  if (
    !PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!", kwlist,
      &PyBobIpGaborJetType, &jet1,
      &PyBobIpGaborJetType, &jet2
    )
  ){
    similarity_doc.print_usage();
    return 0;
  }

  auto j1_ = make_safe(jet1), j2_ = make_safe(jet2);
//  Py_INCREF(jet1);
//  Py_INCREF(jet2);

  try {
    const auto& disp = self->cxx->disparity(*jet1->cxx, *jet2->cxx);
    return Py_BuildValue("(dd)", disp[0], disp[1]);
  }
  catch (std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return 0;
  }
  catch (...) {
    PyErr_Format(PyExc_RuntimeError, "%s cannot compute Gabor jet similarity: unknown exception caught", Py_TYPE(self)->tp_name);
    return 0;
  }
}


static auto load_doc = bob::extension::FunctionDoc(
  "load",
  "Loads the parametrization of the Gabor jet similarity from the given HDF5 file",
  0,
  true
)
.add_prototype("hdf5")
.add_parameter("hdf5", "xbob.io.HDF5File", "An HDF5 file opened for reading")
;

static PyObject* PyBobIpGaborJetSimilarity_load(PyBobIpGaborJetSimilarityObject* self, PyObject* args, PyObject* kwargs) {
  // get list of arguments
  char* kwlist[] = {c("hdf5"), NULL};
  PyBobIoHDF5FileObject* file = 0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&", kwlist,
        PyBobIoHDF5File_Converter, &file
  )){
    load_doc.print_usage();
    return NULL;
  }

  auto file_ = make_safe(file);
  try{
    self->cxx->load(*file->f);
  } catch (std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return 0;
  }catch (...) {
    PyErr_Format(PyExc_RuntimeError, "%s cannot load parametrization: unknown exception caught", Py_TYPE(self)->tp_name);
    return 0;
  }
  Py_RETURN_NONE;
}


static auto save_doc = bob::extension::FunctionDoc(
  "save",
  "Saves the parametrization of this Gabor jet similarity to the given HDF5 file",
  0,
  true
)
.add_prototype("hdf5")
.add_parameter("hdf5", ":py:class:`bob.io.base.HDF5File`", "An HDF5 file open for writing")
;

static PyObject* PyBobIpGaborJetSimilarity_save(PyBobIpGaborJetSimilarityObject* self, PyObject* args, PyObject* kwargs) {
  // get list of arguments
  char* kwlist[] = {c("hdf5"), NULL};
  PyBobIoHDF5FileObject* file = 0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&", kwlist,
        PyBobIoHDF5File_Converter, &file
    )
  ){
    save_doc.print_usage();
    return NULL;
  }

  auto file_ = make_safe(file);
  try{
    self->cxx->save(*file->f);
  } catch (std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return 0;
  }catch (...) {
    PyErr_Format(PyExc_RuntimeError, "%s cannot save parametrization: unknown exception caught", Py_TYPE(self)->tp_name);
    return 0;
  }

  Py_RETURN_NONE;
}


static PyMethodDef PyBobIpGaborJetSimilarity_methods[] = {
  {
    similarity_doc.name(),
    (PyCFunction)PyBobIpGaborJetSimilarity_similarity,
    METH_VARARGS|METH_KEYWORDS,
    similarity_doc.doc()
  },
  {
    disparity_doc.name(),
    (PyCFunction)PyBobIpGaborJetSimilarity_disparity,
    METH_VARARGS|METH_KEYWORDS,
    disparity_doc.doc()
  },
  {
    load_doc.name(),
    (PyCFunction)PyBobIpGaborJetSimilarity_load,
    METH_VARARGS|METH_KEYWORDS,
    load_doc.doc()
  },
  {
    save_doc.name(),
    (PyCFunction)PyBobIpGaborJetSimilarity_save,
    METH_VARARGS|METH_KEYWORDS,
    save_doc.doc()
  },
  {0} /* Sentinel */
};


/******************************************************************/
/************ Module Section **************************************/
/******************************************************************/

// Define the Gabor wavelet type struct; will be initialized later
PyTypeObject PyBobIpGaborJetSimilarityType = {
  PyVarObject_HEAD_INIT(0,0)
  0
};

bool init_BobIpGaborJetSimilarity(PyObject* module)
{

  // initialize the Gabor wavelet type struct
  PyBobIpGaborJetSimilarityType.tp_name = Similarity_doc.name();
  PyBobIpGaborJetSimilarityType.tp_basicsize = sizeof(PyBobIpGaborJetSimilarityObject);
  PyBobIpGaborJetSimilarityType.tp_flags = Py_TPFLAGS_DEFAULT;
  PyBobIpGaborJetSimilarityType.tp_doc = Similarity_doc.doc();

  // set the functions
  PyBobIpGaborJetSimilarityType.tp_new = PyType_GenericNew;
  PyBobIpGaborJetSimilarityType.tp_init = reinterpret_cast<initproc>(PyBobIpGaborJetSimilarity_init);
  PyBobIpGaborJetSimilarityType.tp_dealloc = reinterpret_cast<destructor>(PyBobIpGaborJetSimilarity_delete);
  PyBobIpGaborJetSimilarityType.tp_methods = PyBobIpGaborJetSimilarity_methods;
  PyBobIpGaborJetSimilarityType.tp_getset = PyBobIpGaborJetSimilarity_getseters;
  PyBobIpGaborJetSimilarityType.tp_call = reinterpret_cast<ternaryfunc>(PyBobIpGaborJetSimilarity_similarity);

  // check that everyting is fine
  if (PyType_Ready(&PyBobIpGaborJetSimilarityType) < 0)
    return false;

  // add the type to the module
  Py_INCREF(&PyBobIpGaborJetSimilarityType);
  return PyModule_AddObject(module, "Similarity", (PyObject*)&PyBobIpGaborJetSimilarityType) >= 0;
}

