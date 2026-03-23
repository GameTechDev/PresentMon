using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using CppAst;
using Scriban;

namespace StructDumperGenerator
{
    class Program
    {
        static void Main(string[] args)
        {
            var repositoryRoot = FindRepositoryRoot();
            var targets = new List<TargetHeader>
            {
                new(
                    "IGCL",
                    "IntelPresentMon/ControlLib/igcl/igcl_api.h",
                    new HashSet<string>
                    {
                        "ctl_result_t",
                        "ctl_init_args_t",
                        "ctl_device_adapter_properties_t",
                        "ctl_mem_handle_t",
                        "ctl_pwr_handle_t",
                        "ctl_fan_properties_t",
                        "ctl_power_telemetry_t",
                        "ctl_mem_state_t",
                        "ctl_mem_bandwidth_t",
                        "ctl_power_limits_t",
                    }),
                new(
                    "NVAPI",
                    "IntelPresentMon/ControlLib/nvapi/nvapi.h",
                    new HashSet<string>
                    {
                        "NvAPI_Status",
                        "NV_GPU_THERMAL_SETTINGS",
                        "NV_GPU_CLOCK_FREQUENCIES",
                        "NV_GPU_DYNAMIC_PSTATES_INFO_EX",
                    }),
                new(
                    "NVML",
                    "IntelPresentMon/ControlLib/nvml/nvml.h",
                    new HashSet<string>
                    {
                        "nvmlReturn_t",
                        "nvmlPciInfo_t",
                        "nvmlMemory_t",
                    }),
                new(
                    "ADL",
                    "IntelPresentMon/ControlLib/adl/adl_sdk.h",
                    new HashSet<string>
                    {
                        "AdapterInfo",
                        "ADLThermalControllerInfo",
                        "ADLMemoryInfoX4",
                        "ADLTemperature",
                        "ADLFanSpeedInfo",
                        "ADLFanSpeedValue",
                        "ADLPMActivity",
                        "ADLOD6ThermalControllerCaps",
                        "ADLOD6FanSpeedInfo",
                        "ADLOD6CurrentStatus",
                        "ADLOD6Capabilities",
                        "ADLODNCapabilitiesX2",
                        "ADLODNPerformanceStatus",
                        "ADLODNFanControl",
                        "ADLPMLogDataOutput",
                    }),
            };

            Console.WriteLine($"Starting reflection generation for {targets.Count} header(s).");

            var parsedHeaders = new List<ParsedHeader>();
            foreach (var target in targets) {
                var headerPath = Path.GetFullPath(Path.Combine(repositoryRoot, ToSystemPath_(target.IncludePath)));
                if (!File.Exists(headerPath)) {
                    Console.WriteLine($"Target header not found: {headerPath}");
                    return;
                }
                var headerDirectory = Path.GetDirectoryName(headerPath)!;
                Console.WriteLine($"Parsing {target.DisplayName}: {target.IncludePath}");

                var compilation = CppParser.ParseFile(ToClangPath_(headerPath));
                if (compilation.HasErrors) {
                    Console.WriteLine($"Parsing failed for {target.DisplayName}:");
                    foreach (var message in compilation.Diagnostics.Messages) {
                        Console.WriteLine(message);
                    }
                    return;
                }

                parsedHeaders.Add(new ParsedHeader {
                    Header = target,
                    Classes = compilation.Classes
                        .Where(cppClass =>
                            cppClass.IsDefinition &&
                            !cppClass.IsAnonymous &&
                            !string.IsNullOrWhiteSpace(cppClass.Name) &&
                            IsDeclaredUnderRoot_(cppClass, headerDirectory))
                        .ToList(),
                    Enums = compilation.Enums
                        .Where(cppEnum =>
                            !string.IsNullOrWhiteSpace(cppEnum.Name) &&
                            IsDeclaredUnderRoot_(cppEnum, headerDirectory))
                        .ToList(),
                    Typedefs = compilation.Typedefs
                        .Where(cppTypedef =>
                            !string.IsNullOrWhiteSpace(cppTypedef.Name) &&
                            IsDeclaredUnderRoot_(cppTypedef, headerDirectory))
                        .ToList(),
                });
            }

            var classesByName = BuildNameMap(
                parsedHeaders.SelectMany(parsedHeader => parsedHeader.Classes),
                cppClass => cppClass.Name);
            var enumsByName = BuildNameMap(
                parsedHeaders.SelectMany(parsedHeader => parsedHeader.Enums),
                cppEnum => cppEnum.Name);
            var typedefsByName = BuildNameMap(
                parsedHeaders.SelectMany(parsedHeader => parsedHeader.Typedefs),
                cppTypedef => cppTypedef.Name);

            var includedStructs = new HashSet<string>(StringComparer.Ordinal);
            var includedEnums = new HashSet<string>(StringComparer.Ordinal);
            var rootedTypedefSet = new HashSet<string>(StringComparer.Ordinal);
            var rootedTypedefNames = new List<string>();

            foreach (var rootTypeName in targets.SelectMany(target => target.RootTypeNames).Distinct()) {
                if (!IncludeRootType(
                    rootTypeName,
                    classesByName,
                    enumsByName,
                    typedefsByName,
                    includedStructs,
                    includedEnums,
                    rootedTypedefSet,
                    rootedTypedefNames)) {
                    Console.WriteLine($"Warning: rooted dump type not found: {rootTypeName}");
                }
            }

            var typedefs = rootedTypedefNames
                .Select(name => ProcessTypedef(typedefsByName[name]))
                .Where(info => info != null)
                .Cast<TypedefInfo>()
                .ToList();
            var structs = parsedHeaders
                .SelectMany(parsedHeader => parsedHeader.Classes)
                .Where(cppClass => includedStructs.Contains(cppClass.Name))
                .Select(ProcessStruct)
                .ToList();
            var enumsOut = parsedHeaders
                .SelectMany(parsedHeader => parsedHeader.Enums)
                .Where(cppEnum => includedEnums.Contains(cppEnum.Name))
                .Select(ProcessEnum)
                .ToList();

            Console.WriteLine(
                $"Reachable dump surface: {typedefs.Count} rooted typedef(s), {structs.Count} type dumper(s), {enumsOut.Count} enum dumper(s).");

            var templatePath = Path.Combine(repositoryRoot, "Reflector", "StructDumpers.h.scriban");
            Console.WriteLine($"Loading template: {MakeRepositoryRelativePath_(repositoryRoot, templatePath)}");
            var templateText = File.ReadAllText(templatePath);
            var template = Template.Parse(templateText);
            if (template.HasErrors) {
                Console.WriteLine("Template parse failed:");
                foreach (var message in template.Messages) {
                    Console.WriteLine(message);
                }
                return;
            }

            var model = new {
                includes = targets.Select(target => target.IncludePath).Distinct().ToList(),
                typedefs = typedefs,
                structs = structs,
                enums = enumsOut,
            };

            var result = template.Render(model, member => member.Name);

            var outputPath = Path.Combine(
                repositoryRoot,
                "IntelPresentMon",
                "CommonUtilities",
                "ref",
                "gen",
                "GeneratedReflection.h");
            Directory.CreateDirectory(Path.GetDirectoryName(outputPath)!);
            File.WriteAllText(outputPath, result);

            Console.WriteLine($"Wrote {MakeRepositoryRelativePath_(repositoryRoot, outputPath)}");
            Console.WriteLine("Code generation complete.");
        }

        static Dictionary<string, T> BuildNameMap<T>(IEnumerable<T> declarations, Func<T, string> getName)
        {
            var map = new Dictionary<string, T>(StringComparer.Ordinal);
            foreach (var declaration in declarations) {
                var name = getName(declaration);
                if (string.IsNullOrWhiteSpace(name) || map.ContainsKey(name)) {
                    continue;
                }
                map.Add(name, declaration);
            }
            return map;
        }

        static bool IncludeRootType(
            string rootTypeName,
            IReadOnlyDictionary<string, CppClass> classesByName,
            IReadOnlyDictionary<string, CppEnum> enumsByName,
            IReadOnlyDictionary<string, CppTypedef> typedefsByName,
            HashSet<string> includedStructs,
            HashSet<string> includedEnums,
            HashSet<string> rootedTypedefSet,
            List<string> rootedTypedefNames)
        {
            if (classesByName.TryGetValue(rootTypeName, out var cppClass)) {
                IncludeReferencedType(cppClass, includedStructs, includedEnums);
                return true;
            }
            if (enumsByName.TryGetValue(rootTypeName, out var cppEnum)) {
                includedEnums.Add(cppEnum.Name);
                return true;
            }
            if (!typedefsByName.TryGetValue(rootTypeName, out var cppTypedef)) {
                return false;
            }

            var unwrappedType = UnwrapType(cppTypedef);
            if (unwrappedType is CppClass typedefClass) {
                IncludeReferencedType(typedefClass, includedStructs, includedEnums);
            }
            else if (unwrappedType is CppEnum typedefEnum) {
                includedEnums.Add(typedefEnum.Name);
            }
            else if (rootedTypedefSet.Add(cppTypedef.Name)) {
                rootedTypedefNames.Add(cppTypedef.Name);
            }

            IncludeReferencedType(cppTypedef.ElementType, includedStructs, includedEnums);
            return true;
        }

        static void IncludeReferencedType(
            CppType type,
            HashSet<string> includedStructs,
            HashSet<string> includedEnums)
        {
            switch (type) {
            case CppQualifiedType qualifiedType:
                IncludeReferencedType(qualifiedType.ElementType, includedStructs, includedEnums);
                break;
            case CppTypedef typedefType:
                IncludeReferencedType(typedefType.ElementType, includedStructs, includedEnums);
                break;
            case CppArrayType arrayType:
                IncludeReferencedType(arrayType.ElementType, includedStructs, includedEnums);
                break;
            case CppClass cppClass when cppClass.IsDefinition && !string.IsNullOrWhiteSpace(cppClass.Name):
                if (!includedStructs.Add(cppClass.Name)) {
                    return;
                }
                foreach (var field in cppClass.Fields.Where(field => !string.IsNullOrWhiteSpace(field.Name))) {
                    IncludeReferencedType(field.Type, includedStructs, includedEnums);
                }
                break;
            case CppEnum cppEnum when !string.IsNullOrWhiteSpace(cppEnum.Name):
                includedEnums.Add(cppEnum.Name);
                break;
            }
        }

        static TypedefInfo? ProcessTypedef(CppTypedef cppTypedef)
        {
            var unwrappedType = UnwrapType(cppTypedef);
            if (unwrappedType is CppClass || unwrappedType is CppEnum) {
                return null;
            }

            return new TypedefInfo {
                Name = cppTypedef.Name,
                DumpExpression = GetDumpExpression(cppTypedef, "s"),
            };
        }

        static StructInfo ProcessStruct(CppClass cppClass)
        {
            var structInfo = new StructInfo {
                Name = cppClass.Name,
                Members = new List<MemberInfo>(),
                Type = cppClass.ClassKind == CppClassKind.Class ? "class" :
                    cppClass.ClassKind == CppClassKind.Union ? "union" :
                    "struct",
            };

            foreach (var field in cppClass.Fields.Where(field => !string.IsNullOrWhiteSpace(field.Name))) {
                structInfo.Members.Add(new MemberInfo {
                    Name = field.Name,
                    Type = field.Type.GetDisplayName(),
                    DumpExpression = GetDumpExpression(field.Type, $"s.{field.Name}"),
                });
            }

            return structInfo;
        }

        static EnumInfo ProcessEnum(CppEnum cppEnum)
        {
            return new EnumInfo {
                Name = cppEnum.Name,
                Values = cppEnum.Items
                    .Where(item => !string.IsNullOrWhiteSpace(item.Name))
                    .DistinctBy(item => item.Value)
                    .Select(item => new EnumValue {
                        Name = item.Name,
                    })
                    .ToList(),
            };
        }

        static string GetDumpExpression(CppType type, string variableAccess)
        {
            var uniqueId = 0;
            return GetDumpExpression(type, variableAccess, ref uniqueId);
        }

        static string GetDumpExpression(CppType type, string variableAccess, ref int uniqueId)
        {
            var unwrappedType = UnwrapType(type);

            if (unwrappedType is CppPrimitiveType cppPrim) {
                if (cppPrim.FullName == "char" || cppPrim.FullName == "unsigned char") {
                    return $"(int){variableAccess}";
                }
                return variableAccess;
            }
            else if (unwrappedType is CppClass cppClass) {
                if (cppClass.IsAnonymous || string.IsNullOrWhiteSpace(cppClass.Name)) {
                    return BuildInlineClassDumpExpression(cppClass, variableAccess, ref uniqueId);
                }
                return $"DumpGenerated({variableAccess})";
            }
            else if (unwrappedType is CppEnum cppEnum) {
                if (string.IsNullOrWhiteSpace(cppEnum.Name)) {
                    return $"(int){variableAccess}";
                }
                return $"DumpGenerated({variableAccess})";
            }
            else if (unwrappedType is CppArrayType arrayType) {
                var elementType = UnwrapType(arrayType.ElementType);
                if (elementType.FullName == "char") {
                    return variableAccess;
                }
                else {
                    return BuildInlineArrayDumpExpression(arrayType.ElementType, variableAccess, ref uniqueId);
                }
            }
            else if (unwrappedType is CppPointerType) {
                return $"({variableAccess} ? std::format(\"0x{{:016X}}\", reinterpret_cast<std::uintptr_t>({variableAccess})) : \"null\"s)";
            }
            return "\"{ unsupported }\"";
        }

        static string BuildInlineClassDumpExpression(CppClass cppClass, string variableAccess, ref int uniqueId)
        {
            var ossVar = $"oss_{uniqueId++}";
            var fieldExpressions = new List<string>();
            foreach (var field in cppClass.Fields.Where(field => !string.IsNullOrWhiteSpace(field.Name))) {
                fieldExpressions.Add(
                    $"{ossVar} << \" .{field.Name} = \" << {GetDumpExpression(field.Type, $"{variableAccess}.{field.Name}", ref uniqueId)};");
            }
            var classTypeName = cppClass.ClassKind == CppClassKind.Class ? "class" :
                cppClass.ClassKind == CppClassKind.Union ? "union" :
                "struct";

            var body = string.Join(" ", fieldExpressions);
            return $"([&]() {{ std::ostringstream {ossVar}; {ossVar} << std::boolalpha << \"{classTypeName} {{\"; {body} {ossVar} << \" }}\"; return {ossVar}.str(); }}())";
        }

        static string BuildInlineArrayDumpExpression(CppType elementType, string variableAccess, ref int uniqueId)
        {
            var elementVar = $"elem_{uniqueId++}";
            var elementExpression = GetDumpExpression(elementType, elementVar, ref uniqueId);
            return $"DumpArray_({variableAccess}, [&](const auto& {elementVar}) {{ return {elementExpression}; }})";
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

        static string FindRepositoryRoot()
        {
            foreach (var candidate in new[] { Directory.GetCurrentDirectory(), AppContext.BaseDirectory }) {
                var current = new DirectoryInfo(candidate);
                while (current != null) {
                    if (File.Exists(Path.Combine(current.FullName, "PresentMon.sln")) &&
                        File.Exists(Path.Combine(current.FullName, "Reflector", "StructDumpers.h.scriban"))) {
                        return current.FullName;
                    }
                    current = current.Parent;
                }
            }

            throw new DirectoryNotFoundException("Unable to locate repository root.");
        }

        static string ToSystemPath_(string path)
        {
            return path.Replace('/', Path.DirectorySeparatorChar);
        }

        static string ToClangPath_(string path)
        {
            return Path.GetFullPath(path).Replace(Path.DirectorySeparatorChar, '/');
        }

        static string MakeRepositoryRelativePath_(string repositoryRoot, string path)
        {
            return Path.GetRelativePath(repositoryRoot, path).Replace(Path.DirectorySeparatorChar, '/');
        }

        static bool IsDeclaredUnderRoot_(CppElement element, string rootPath)
        {
            var sourceFile = element.SourceFile ?? element.Span.Start.File;
            if (string.IsNullOrWhiteSpace(sourceFile)) {
                return false;
            }

            var normalizedRoot = Path.GetFullPath(rootPath)
                .TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar) +
                Path.DirectorySeparatorChar;
            var normalizedSourceFile = Path.GetFullPath(sourceFile);
            return normalizedSourceFile.StartsWith(normalizedRoot, StringComparison.OrdinalIgnoreCase);
        }
    }

    class TargetHeader
    {
        public TargetHeader(string displayName, string includePath, HashSet<string> rootTypeNames)
        {
            DisplayName = displayName;
            IncludePath = includePath;
            RootTypeNames = rootTypeNames;
        }

        public string DisplayName { get; }
        public string IncludePath { get; }
        public HashSet<string> RootTypeNames { get; }
    }

    class ParsedHeader
    {
        public required TargetHeader Header { get; set; }
        public required List<CppClass> Classes { get; set; }
        public required List<CppEnum> Enums { get; set; }
        public required List<CppTypedef> Typedefs { get; set; }
    }

    class TypedefInfo
    {
        public required string Name { get; set; }
        public required string DumpExpression { get; set; }
    }

    class StructInfo
    {
        public required string Name { get; set; }
        public required string Type { get; set; }
        public required List<MemberInfo> Members { get; set; }
    }

    class MemberInfo
    {
        public required string Name { get; set; }
        public required string Type { get; set; }
        public required string DumpExpression { get; set; }
    }

    class EnumInfo
    {
        public required string Name { get; set; }
        public required List<EnumValue> Values { get; set; }
    }

    class EnumValue
    {
        public required string Name { get; set; }
    }
}
