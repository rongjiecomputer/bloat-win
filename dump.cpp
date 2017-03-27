#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <algorithm>

void printHelp(const char* program) {
  printf("USAGE: %s [options]\n\n", program);
  puts(
      "Options:\n"
      "  -in filename\n"
      "    Specify text file generated by SymbolSort\n"
      "  -out filename\n"
      "    Write output to specified file. Default is dump.json\n");
  exit(0);
}

void Fatal(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  fflush(stdout);
  fflush(stderr);
  exit(0);
}

FILE *input = nullptr, *output = nullptr;

void parseArgs(int argc, char** argv) {
  if (argc < 2)
    printHelp(argv[0]);
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (strcmp(argv[i], "-in") == 0) {
        if (i + 1 < argc) {
          i++;
          input = fopen(argv[i], "r");
        } else {
          Fatal("Missing filename for '-in'\n");
        }
      } else if (strcmp(argv[i], "-out") == 0) {
        if (i + 1 < argc) {
          i++;
          output = fopen(argv[i], "w");
        } else {
          Fatal("Missing filename for '-out'\n");
        }
      } else {
        Fatal("Unknown option: %s\n", argv[i]);
      }
    } else {
      Fatal("Unknown argument: %s\n", argv[i]);
    }
  }
  if (input == nullptr)
    Fatal("No input file specified\n");
  if (output == nullptr)
    output = fopen("dump.json", "w");
}

struct Entry {
  std::string path;
  uint64_t size;
};

template <class ForwardIterator>
void normalizePath(ForwardIterator start, ForwardIterator end) {
  while (start != end) {
    if (*start == '\\')
      *start = '/';
    start++;
  }
}

template <class ForwardIterator>
int commonParent(ForwardIterator a, ForwardIterator b) {
  int p = 0;
  while (*a && *b && *a == *b) {
    if (*a == '/')
      p++;
    a++;
    b++;
  }
  return p;
}

std::string extractStem(std::string path) {
  std::string stem;
  int pos = path.length() - 1;
  if (path[pos] != '/')
    stem += path[pos];
  pos--;
  while (pos >= 0 && path[pos] != '/') {
    stem += path[pos--];
  }
  std::reverse(stem.begin(), stem.end());
  return stem;
}

bool isSource(char* end) {
  return strncmp(".cc", end - 3, 3) == 0 || strncmp(".h", end - 2, 2) == 0 ||
         strncmp(".cpp", end - 4, 4) == 0 || strncmp(".c", end - 2, 2) == 0 ||
         strncmp(".asm", end - 4, 4) == 0 || strncmp(".inl", end - 4, 4) == 0;
}

void printIndent(int indent, const char* fmt, ...) {
  for (int i = 0; i < indent; i++)
    fputs("  ", output);
  va_list args;
  va_start(args, fmt);
  vfprintf(output, fmt, args);
  va_end(args);
}

int RunMain() {
  bool found = false;
  char buffer[300];

  constexpr char kMarker[] = "Sorted by Path";
  while (fgets(buffer, 300, input) != nullptr) {
    if (strncmp(kMarker, buffer, sizeof(kMarker) - 1) == 0) {
      found = true;
      break;
    }
  }
  if (!found)
    Fatal("\'Sorted by Path' table not found\n");

  fgets(buffer, 300, input); // ignore table header

  std::vector<Entry> v;
  char path[300];
  v.push_back({"", 0});

  while (fgets(buffer, 300, input) != nullptr) {
    int size, count;
    if (sscanf(buffer, " %d %d %s", &size, &count, path) != 3) {
      break;
    }
    int n = strlen(path);
    normalizePath(path, path + n);
    if (!isSource(path + n)) {
      path[n++] = '/';
      path[n] = '\0';
    }
    v.push_back({path, static_cast<uint64_t>(size)});
  }

  std::sort(v.begin(), v.end(), [](const Entry& lhs, const Entry& rhs) {
    return std::lexicographical_compare(lhs.path.begin(), lhs.path.end(),
                                        rhs.path.begin(), rhs.path.end());
  });

  int total = 0;
  int lastIndent = 0;

  fputs(
      "var kTree = {\n"
      "  \"children\": [\n",
      output);

  for (int i = 1; i < v.size(); i++) {
    int indent = 2 * commonParent(v[i - 1].path.begin(), v[i].path.begin()) + 2;
    if (indent == 2)
      total += v[i].size;

    for (int j = lastIndent; j >= indent; j -= 2) {
      printIndent(j + 1, "]\n");
      printIndent(j, "},\n");
    }

    printIndent(indent, "{\n");
    printIndent(indent, "  \"data\": {\n");
    printIndent(indent, "    \"$area\": %I64u\n", v[i].size);
    printIndent(indent, "  },\n");
    printIndent(indent, "  \"name\": \"%s (%.2fKB)\",\n",
                extractStem(v[i].path).c_str(),
                static_cast<double>(v[i].size) / 1024.0);
    printIndent(indent, "  \"children\": [\n");

    lastIndent = indent;
  }

  for (int i = lastIndent; i >= 2; i -= 2) {
    printIndent(i + 1, "]\n");
    printIndent(i, "},\n");
  }

  fprintf(output,
          "  ],\n"
          "  \"data\": {\n"
          "    \"$area\": %d\n"
          "  },\n"
          "  \"name\": \"full (%.2fKB)\"\n"
          "};\n",
          total, static_cast<double>(total) / 1024.0);

  fclose(input);
  fclose(output);

  return 0;
}

int main(int argc, char** argv) {
  parseArgs(argc, argv);
  return RunMain();
}
