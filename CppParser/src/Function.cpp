//
// Function.cpp
//
// $Id: //poco/1.4/CppParser/src/Function.cpp#2 $
//
// Library: CppParser
// Package: SymbolTable
// Module:  Function
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/CppParser/Function.h"
#include "Poco/CppParser/Parameter.h"
#include "Poco/CppParser/NameSpace.h"
#include "Poco/CppParser/Struct.h"
#include "Poco/CppParser/Utility.h"
#include "Poco/String.h"
#include <sstream>
#include <cctype>
#include <cstddef>


namespace Poco {
namespace CppParser {


Function::Function(const std::string& decl, NameSpace* pNameSpace):
	Decl(decl, pNameSpace),
	_flags(0),
	_retParam()
{
	if (hasAttr(decl, "static"))
		_flags |= FN_STATIC;
	if (hasAttr(decl, "virtual"))
		_flags |= FN_VIRTUAL;
	if (hasAttr(decl, "inline"))
		_flags |= FN_INLINE;
	if (hasAttr(decl, "template"))
		_flags |= FN_TEMPLATE;

	if (isMethod() || isFunction())
	{
		// parse the decl
		std::size_t pos = decl.rfind(name());
		_retParam = decl.substr(0, pos-1);
		// eliminate static, virtual, inline, template
		_retParam = replace(_retParam, "static ", "");
		_retParam = replace(_retParam, "virtual ", "");
		_retParam = replace(_retParam, "inline ", "");
		if (_flags & FN_TEMPLATE)
		{
			std::size_t pos = _retParam.find(">");
			poco_assert (pos != std::string::npos);
			_retParam = _retParam.substr(pos+1);
		}
		Poco::trimInPlace(_retParam);
	}
}


Function::~Function()
{
	for (Parameters::iterator it = _params.begin(); it != _params.end(); ++it)
	{
		delete *it;
	}
}


void Function::addParameter(Parameter* pParam)
{
	_params.push_back(pParam);
}


Function::Iterator Function::begin() const
{
	return _params.begin();
}


Function::Iterator Function::end() const
{
	return _params.end();
}


void Function::makeInline()
{
	_flags |= FN_INLINE;
}


void Function::makeConst()
{
	_flags |= FN_CONST;
}


void Function::makePureVirtual()
{
	_flags |= FN_PURE_VIRTUAL;
}


void Function::makeFinal()
{
	_flags |= FN_FINAL;
}

	
void Function::makeOverride()
{
	_flags |= FN_OVERRIDE;
}

	
void Function::makeNoexcept()
{
	_flags |= FN_NOEXCEPT;
}


void Function::makeDefault()
{
	_flags |= FN_DEFAULT;
}


void Function::makeDelete()
{
	_flags |= FN_DELETE;
}


bool Function::isConstructor() const
{
	return name() == nameSpace()->name();
}

	
bool Function::isDestructor() const
{
	return name()[0] == '~';
}


bool Function::isMethod() const
{
	return !isConstructor() && !isDestructor() && nameSpace()->kind() == Symbol::SYM_STRUCT;
}


bool Function::isFunction() const
{
	return nameSpace()->kind() == Symbol::SYM_NAMESPACE;
}


int Function::countParameters() const
{
	return (int) _params.size();
}


Symbol::Kind Function::kind() const
{
	return Symbol::SYM_FUNCTION;
}


std::string Function::signature() const
{
	std::string signature(declaration());
	if (signature.compare(0, 8, "virtual ") == 0)
		signature.erase(0, 8);
	else if (signature.compare(0, 7, "static ") == 0)
		signature.erase(0, 8);
	if (signature.compare(0, 7, "inline ") == 0)
		signature.erase(0, 7);
	signature += "(";
	bool isFirst = true;
	for (Iterator it = begin(); it != end(); ++it)
	{
		if (isFirst)
			isFirst = false;
		else
			signature += ", ";
		std::string arg = (*it)->declaration();
		std::string::size_type pos = arg.size() - 1;
		while (pos > 0 && !std::isspace(arg[pos])) --pos;
		while (pos > 0 && std::isspace(arg[pos])) --pos;
		signature.append(arg, 0, pos + 1);
	}
	signature += ")";
	if (_flags & FN_CONST)
		signature += " const";
	return signature;
}

	
bool Function::isVirtual() const
{
	if (_flags & FN_VIRTUAL)
	{
		return true;
	}
	else if (isDestructor())
	{
		Struct* pClass = dynamic_cast<Struct*>(nameSpace());
		return pClass && pClass->hasVirtualDestructor();
	}
	else return getOverridden() != 0;
	return false;
}


Function* Function::getOverridden() const
{
	if (isMethod() && !(_flags & FN_STATIC))
	{
		Struct* pClass = dynamic_cast<Struct*>(nameSpace());
		if (pClass)
		{
			for (Struct::BaseIterator it = pClass->baseBegin(); it != pClass->baseEnd(); ++it)
			{
				if (it->pClass)
				{
					Function* pOverridden = it->pClass->findFunction(signature());
					if (pOverridden && pOverridden->isVirtual())
						return pOverridden;
				}
			}
		}
	}
	return 0;
}


std::string Function::toString() const
{
	std::ostringstream ostr;
	ostr << Decl::toString() << "(\n";
	for (Iterator it = begin(); it != end(); ++it)
	{
		ostr << "\t" << (*it)->toString() << "\n";
	}
	ostr << ");";
	return ostr.str();
}


} } // namespace Poco::CppParser