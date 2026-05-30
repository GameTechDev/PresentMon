using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using CppAst;
using Scriban;
using Scriban.Runtime;

namespace StructDumperGenerator
{
    class Program
    {
        static void Main(string[] args)
        {
            // List of structs and enums with their header files
            var targetHeaders = new List<string>
            {
                "IntelPresentMon/ControlLib/igcl_api.h",
            };

            var compilation = CppParser.ParseFiles(targetHeaders);

            if (compilation.HasErrors) {
                foreach (var message in compilation.Diagnostics.Messages) {
                    Console.WriteLine(message);
                }
                return;
            }

            // Collect structs, enums, and their members
            var structs = new List<StructInfo>();
            var enums = new List<EnumInfo>();

            // iterate over all structs (classes, unions)
            foreach (var cppClass in compilation.Classes) {
                if (cppClass.IsDefinition) {
                    ProcessStruct(cppClass, compilation, structs);
                }
            }

            // iterate over all enums
            foreach (var cppEnum in compilation.Enums) {
                ProcessEnum(cppEnum, compilation, enums);
            }

            // Load the Scriban template
            var templateText = File.ReadAllText("Reflector\\StructDumpers.h.scriban");
            var template = Template.Parse(templateText);

            // Prepare the model for the template
            var model = new {
                includes = new HashSet<string>(targetHeaders),
                structs = structs,
                enums = enums
            };

            // Render the template
            var result = template.Render(model, member => member.Name);

            // Output the generated code to a file
            File.WriteAllText("IntelPresentMon\\CommonUtilities\\ref\\gen\\GeneratedReflection.h", result);

            Console.WriteLine("Code generation complete.");
        }

        // Recursive method to process structs
        static void ProcessStruct(CppClass cppClass, CppCompilation compilation, List<StructInfo> structs)
        {
            var structInfo = new StructInfo {
                Name = cppClass.Name,
                Members = new List<MemberInfo>(),
                Type =  cppClass.ClassKind == CppClassKind.Class ? "class" :
                        cppClass.ClassKind == CppClassKind.Union ? "union" :
                        "struct"
            };

            foreach (var field in cppClass.Fields) {
                var fieldType = UnwrapType(field.Type);
                var memberInfo = new MemberInfo {
                    Name = field.Name,
                    Type = field.Type.GetDisplayName(),
                    DumpExpression = GetDumpExpression(field.Type, $"s.{field.Name}")
                };
                structInfo.Members.Add(memberInfo);
            }

            structs.Add(structInfo);
        }

        // Method to process enums
        static void ProcessEnum(CppEnum cppEnum, CppCompilation compilation, List<EnumInfo> enums)
        {
            
            var enumInfo = new EnumInfo {
                Name = cppEnum.Name,
                Values = cppEnum.Items.DistinctBy(item => item.Value).Select(item => new EnumValue {
                    Name = item.Name
                }).ToList()
            };

            enums.Add(enumInfo);
        }

        // Helper method to generate the dump expression for a field
        static string GetDumpExpression(CppType type, string variableAccess)
        {
            var unwrappedType = UnwrapType(type);

            if (unwrappedType is CppPrimitiveType cppPrim) {
                if (cppPrim.FullName == "char" || cppPrim.FullName == "unsigned char") {
                    return $"(int){variableAccess}";
                }
                return variableAccess;
            }
            else if (unwrappedType is CppClass) {
                return $"DumpGenerated({variableAccess})";
            }
            else if (unwrappedType is CppEnum) {
                return $"DumpGenerated({variableAccess})";
            }
            else if (unwrappedType is CppArrayType arrayType) {
                var elementType = UnwrapType(arrayType.ElementType);
                // special case for char arrays, dump as c-string
                if (elementType.FullName == "char") {
                    return variableAccess;
                }
                // sized arrays can be displayed
                else {
                    var isPrimitiveString = elementType is CppPrimitiveType ? "true" : "false";
                    var name = arrayType.ElementType is CppTypedef typedef ? typedef.Name : arrayType.ElementType.FullName;
                    return $"DumpArray_<{name}, {arrayType.Size}, {isPrimitiveString}>({variableAccess})";
                }
            }
            else if (unwrappedType is CppPointerType pointerType) {
                return $"({variableAccess} ? std::format(\"0x{{:016X}}\", reinterpret_cast<std::uintptr_t>({variableAccess})) : \"null\"s)";
            }
            return "\"{ unsupported }\"";
        }

        static CppType UnwrapType(CppType type)
        {
            while (true) {
                if (type is CppQualifiedType qualifiedType) {
                    type = qualifiedType.ElementType;
                }
                else if (type is CppTypedef typedefType) {
                    type = typedefType.ElementType;
                }
                else {
                    break;
                }
            }
            return type;
        }
    }

    // Class to represent a header file and its target structs/enums
    class HeaderStruct
    {
        public string HeaderFile { get; }
        public HashSet<string> StructNames { get; }

        public HeaderStruct(string headerFile, HashSet<string> structNames)
        {
            HeaderFile = headerFile;
            StructNames = structNames;
        }
    }

    // Class to hold struct information
    class StructInfo
    {
        public required string Name { get; set; }
        public required string Type {  get; set; }
        public required List<MemberInfo> Members { get; set; }
    }

    // Class to hold member information
    class MemberInfo
    {
        public required string Name { get; set; }
        public required string Type { get; set; }
        public required string DumpExpression { get; set; }
    }

    // Class to hold enum information
    class EnumInfo
    {
        public required string Name { get; set; }
        public required List<EnumValue> Values { get; set; }
    }

    // Class to hold enum value information
    class EnumValue
    {
        public required string Name { get; set; }
    }
}
