#include "../include/module.h"
#include "../include/ModuleParser.h"
#include <iostream>

namespace llvm_python {

    struct PythonTypes {
        py::object IRPyModule;
        py::object ModulePyClass;
        py::object FunctionPyClass;
        py::object BlockPyClass;
        py::object ArgumentPyClass;
        py::object TypePyClass;
        py::object ValuePyClass;
        py::object InstructionPyClass;
        py::object AttributePyClass;
    };

    py::object handleType(Type *type, const PythonTypes &PT)
    {
        py::object typeObject = PT.TypePyClass();
        std::string buffer;
        raw_string_ostream OS(buffer);
        type->print(OS);
        typeObject.attr("name") = py::cast(std::move(OS.str()));
        typeObject.attr("type_id") = py::cast((int) type->getTypeID());
        return typeObject;
    }

    py::object handleAttribute(const Attribute &attribute, const PythonTypes &PT)
    {
        py::object attributeObject = PT.AttributePyClass();
        if (attribute.isIntAttribute())
        {
            attributeObject.attr("value_int") = py::cast((int)attribute.getValueAsInt());
        }
        else if (attribute.isStringAttribute())
        {
            attributeObject.attr("value_string") = py::cast(attribute.getKindAsString().str());
        }
        else if (attribute.isEnumAttribute())
        {
            attributeObject.attr("value_enum") = py::cast((int )attribute.getKindAsEnum());
        }
        else if (attribute.isTypeAttribute())
        {
            attributeObject.attr("value_type") = handleType(attribute.getValueAsType(), PT);
        }
//        attributeObject.attr("value") = py::cast(attribute.getValueAsInt());
//        a.attr("kind_str") = py::cast(attribute.getValueAsString().str());

        return attributeObject;
    }

    py::object handleInstruction(Instruction &instruction, const PythonTypes &PT)
    {
        py::object instructionObject = PT.InstructionPyClass();
        instructionObject.attr("op_code") = py::cast(instruction.getOpcode());
        instructionObject.attr("op_code_name") = py::cast(instruction.getOpcodeName());
        instructionObject.attr("type") = handleType(instruction.getType(), PT);
        return instructionObject;
    }

    py::object handleBlock(BasicBlock &block, const PythonTypes &PT)
    {
        py::object blockObject = PT.BlockPyClass();
        blockObject.attr("name") = py::cast(block.getName().str());
        blockObject.attr("type") = handleType(block.getType(), PT);

        std::vector<py::object> instructions;

        for (Instruction &instruction: block) {
            py::object instructionObject = handleInstruction(instruction, PT);
            instructionObject.attr("block") = blockObject;
            instructions.push_back(instructionObject);
        }
        return blockObject;
    }

    py::object handleValue(Value *value, const PythonTypes &PT)
    {
        py::object valueObject = PT.ValuePyClass();
        valueObject.attr("name") = value->getName().str();
        return valueObject;
    }

    py::object handleArgument(Argument &argument, const PythonTypes &PT)
    {
        py::object argumentObject = PT.ArgumentPyClass();
        argumentObject.attr("name") = py::cast(argument.getName().str());
        argumentObject.attr("position") = py::cast(argument.getArgNo());
        argumentObject.attr("type") = handleType(argument.getType(), PT);
        return argumentObject;
    }

    py::object handleFunction(Function &function, const PythonTypes &PT)
    {
        py::object functionObject = PT.FunctionPyClass();
        functionObject.attr("name") = py::cast(function.getName().str());
        functionObject.attr("return_type") = handleType(function.getReturnType(), PT);

        std::vector<py::object> args;

        for (Argument &arg: function.args()) {
            py::object argumentObject = handleArgument(arg, PT);
            argumentObject.attr("function") = functionObject;
            args.push_back(argumentObject);
        }
        std::vector<py::object> blocks;

        std::vector<py::object> attributes;

        for (const AttributeSet &attributeSet: function.getAttributes())
        {
            std::vector<py::object> attributeSetObject;

            for (const Attribute &attribute: attributeSet)
            {
                py::object attributeObject = handleAttribute(attribute, PT);
                attributeSetObject.push_back(attributeObject);
            }
            attributes.push_back(py::make_tuple(attributeSetObject));
        }

        functionObject.attr("attributes") = py::make_tuple(attributes);

        for (BasicBlock &block: function)
        {
            blocks.push_back(handleBlock(block, PT));
        }

        functionObject.attr("args") = py::cast(args);
        functionObject.attr("name") = py::cast(function.getName().str());
        return functionObject;
    }

    py::object handleModule(Module *module, const PythonTypes &PT)
    {
        py::object moduleObject = PT.ModulePyClass();
        moduleObject.attr("name") = py::cast(module->getName().str());
        std::vector<py::object> functions;

        for (Function &function: *module) {
            std::cout << function.getName().str() << std::endl;
            functions.push_back(handleFunction(function, PT));
        }

        moduleObject.attr("functions") = py::cast(functions);
        return moduleObject;
    }

    py::object createModule(const std::string &IR)
    {
        py::object IRPyModule = py::module_::import("ir");
        PythonTypes PT = PythonTypes{
                py::module_::import("ir"),
                IRPyModule.attr("Module"),
                IRPyModule.attr("Function"),
                IRPyModule.attr("Block"),
                IRPyModule.attr("Argument"),
                IRPyModule.attr("Type"),
                IRPyModule.attr("Value"),
                IRPyModule.attr("Instruction"),
                IRPyModule.attr("Attribute")
        };

        Module *module = parse_module(IR);
        py::object result = handleModule(module, PT);
        delete module;
        return result;
    }

}