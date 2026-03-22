using System;
using System.Collections.Generic;
using System.Globalization;
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
            var targetHeaders = new List<TargetHeader>
            {
                new("IGCL", "IntelPresentMon/ControlLib/igcl/igcl_api.h"),
                new("NVAPI", "IntelPresentMon/ControlLib/nvapi/nvapi.h"),
                new("NVML", "IntelPresentMon/ControlLib/nvml/nvml.h"),
                new("ADL", "IntelPresentMon/ControlLib/adl/adl_sdk.h"),
            };

            Console.WriteLine($"Starting reflection generation for {targetHeaders.Count} header(s).");

            var parsedHeaders = new List<ParsedHeader>();
            for (var i = 0; i < targetHeaders.Count; ++i) {
                var targetHeader = targetHeaders[i];
                var headerPath = Path.Combine(repositoryRoot, ToSystemPath_(targetHeader.IncludePath));
                var headerDirectory = Path.GetDirectoryName(headerPath)!;
                Console.WriteLine($"[{i + 1}/{targetHeaders.Count}] Parsing {targetHeader.DisplayName}: {targetHeader.IncludePath}");

                var compilation = CppParser.ParseFile(headerPath);
                if (compilation.HasErrors) {
                    Console.WriteLine($"Parsing failed for {targetHeader.DisplayName}:");
                    foreach (var message in compilation.Diagnostics.Messages) {
                        Console.WriteLine(message);
                    }
                    return;
                }

                var classes = compilation.Classes
                    .Where(cppClass =>
                        cppClass.IsDefinition &&
                        !cppClass.IsAnonymous &&
                        !string.IsNullOrWhiteSpace(cppClass.Name) &&
                        IsDeclaredUnderRoot_(cppClass, headerDirectory))
                    .ToList();
                var enums = compilation.Enums
                    .Where(cppEnum =>
                        !string.IsNullOrWhiteSpace(cppEnum.Name) &&
                        IsDeclaredUnderRoot_(cppEnum, headerDirectory))
                    .ToList();

                var namedFieldCount = classes.Sum(cppClass => cppClass.Fields.Count(field => !string.IsNullOrWhiteSpace(field.Name)));
                var namedEnumValueCount = enums.Sum(cppEnum => cppEnum.Items.Count(item => !string.IsNullOrWhiteSpace(item.Name)));

                parsedHeaders.Add(new ParsedHeader {
                    Header = targetHeader,
                    Classes = classes,
                    Enums = enums,
                });

                Console.WriteLine(
                    $"[{i + 1}/{targetHeaders.Count}] Parsed {targetHeader.DisplayName}: {classes.Count} type(s), " +
                    $"{enums.Count} enum(s), {namedFieldCount} field(s), {namedEnumValueCount} enum value(s).");
            }

            var totalItems = parsedHeaders.Sum(parsedHeader => parsedHeader.Classes.Count + parsedHeader.Enums.Count);
            Console.WriteLine($"Processing {totalItems} generated item(s) across {parsedHeaders.Count} header(s).");

            var structs = new List<StructInfo>();
            var enumsOut = new List<EnumInfo>();
            var overallProcessedItems = 0;
            var overallFieldCount = 0;
            var overallEnumValueCount = 0;

            for (var headerIndex = 0; headerIndex < parsedHeaders.Count; ++headerIndex) {
                var parsedHeader = parsedHeaders[headerIndex];
                var fileTotalItems = parsedHeader.Classes.Count + parsedHeader.Enums.Count;
                var fileProcessedItems = 0;
                var fileFieldCount = 0;
                var fileEnumValueCount = 0;
                var reportInterval = GetReportInterval_(fileTotalItems);

                foreach (var cppClass in parsedHeader.Classes) {
                    var structInfo = ProcessStruct(cppClass);
                    structs.Add(structInfo);
                    fileFieldCount += structInfo.Members.Count;
                    overallFieldCount += structInfo.Members.Count;
                    ++fileProcessedItems;
                    ++overallProcessedItems;
                    ReportProgress_(
                        parsedHeader.Header,
                        headerIndex,
                        parsedHeaders.Count,
                        fileProcessedItems,
                        fileTotalItems,
                        overallProcessedItems,
                        totalItems,
                        fileFieldCount,
                        fileEnumValueCount,
                        reportInterval,
                        $"type {structInfo.Name}");
                }

                foreach (var cppEnum in parsedHeader.Enums) {
                    var enumInfo = ProcessEnum(cppEnum);
                    enumsOut.Add(enumInfo);
                    fileEnumValueCount += enumInfo.Values.Count;
                    overallEnumValueCount += enumInfo.Values.Count;
                    ++fileProcessedItems;
                    ++overallProcessedItems;
                    ReportProgress_(
                        parsedHeader.Header,
                        headerIndex,
                        parsedHeaders.Count,
                        fileProcessedItems,
                        fileTotalItems,
                        overallProcessedItems,
                        totalItems,
                        fileFieldCount,
                        fileEnumValueCount,
                        reportInterval,
                        $"enum {enumInfo.Name}");
                }

                Console.WriteLine(
                    $"[{headerIndex + 1}/{parsedHeaders.Count}] Completed {parsedHeader.Header.DisplayName}: " +
                    $"{fileProcessedItems}/{fileTotalItems} item(s), {fileFieldCount} field(s), {fileEnumValueCount} enum value(s).");
            }

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
                includes = targetHeaders.Select(header => header.IncludePath).ToList(),
                structs = structs,
                enums = enumsOut,
            };

            Console.WriteLine(
                $"Rendering output: {structs.Count} type dumper(s), {enumsOut.Count} enum dumper(s), " +
                $"{overallFieldCount} field(s), {overallEnumValueCount} enum value(s).");
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
                if (elementType.FullName == "char") {
                    return variableAccess;
                }
                else {
                    return $"DumpArray_({variableAccess})";
                }
            }
            else if (unwrappedType is CppPointerType) {
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

        static int GetReportInterval_(int totalItems)
        {
            return Math.Max(1, totalItems / 20);
        }

        static void ReportProgress_(
            TargetHeader header,
            int headerIndex,
            int headerCount,
            int fileProcessedItems,
            int fileTotalItems,
            int overallProcessedItems,
            int overallTotalItems,
            int fileFieldCount,
            int fileEnumValueCount,
            int reportInterval,
            string lastProcessedItem)
        {
            if (fileProcessedItems != 1 &&
                fileProcessedItems != fileTotalItems &&
                fileProcessedItems % reportInterval != 0) {
                return;
            }

            Console.WriteLine(
                $"[{headerIndex + 1}/{headerCount}] {header.DisplayName}: " +
                $"{fileProcessedItems}/{fileTotalItems} item(s) ({FormatProgress_(fileProcessedItems, fileTotalItems)}), " +
                $"overall {overallProcessedItems}/{overallTotalItems} ({FormatProgress_(overallProcessedItems, overallTotalItems)}), " +
                $"{fileFieldCount} field(s), {fileEnumValueCount} enum value(s), last {lastProcessedItem}");
        }

        static string FormatProgress_(int current, int total)
        {
            if (total <= 0) {
                return "0.0%";
            }
            return ((double)current / total).ToString("0.0%", CultureInfo.InvariantCulture);
        }

        static string ToSystemPath_(string path)
        {
            return path.Replace('/', Path.DirectorySeparatorChar);
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
        public TargetHeader(string displayName, string includePath)
        {
            DisplayName = displayName;
            IncludePath = includePath;
        }

        public string DisplayName { get; }
        public string IncludePath { get; }
    }

    class ParsedHeader
    {
        public required TargetHeader Header { get; set; }
        public required List<CppClass> Classes { get; set; }
        public required List<CppEnum> Enums { get; set; }
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
