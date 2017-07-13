#include <omp.h>
#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <Windows.h>
#include "../redist/some_proc_info.h"

using namespace std;

class matrix {
private:
  double *data;
  uint16_t _width, _height;
public:
  double& operator()(uint16_t x, uint16_t y) {
    if (x > _width || y > _height)
      throw exception("Item index out of bounds");
    return data[_width * y + x];
  }

  matrix& operator=(matrix& M) {
    delete[] data;
    _width = M._width;
    _height = M._height;
    size_t sz = _width*_height;
    data = new double[sz];

    double* end = data + sz;
    for (double* i = data, *j = M.data; i != end; i++, j++)
      *i = *j;
    return *this;
  }

  uint16_t height() { return _height; };
  uint16_t width() { return _width; };

  matrix() {
    _width = 0;
    _height = 0;
    data = nullptr;
  };

  matrix(uint16_t w, uint16_t h) {
    _width = w;
    _height = h;
    size_t sz = _width*_height;
    data = new double[sz];
    generate(data, data + sz, []() -> double {return 0.0; });
  };

  matrix(const matrix &M) {
    _width = M._width;
    _height = M._height;
    size_t sz = _width*_height;
    data = new double[sz];

    double* end = data + sz;
    for (double* i = data, *j = M.data; i != end; i++, j++)
      *i = *j;
  };

  ~matrix() {
    delete[] data;
  }
};

void run_test(size_t size, uint32_t thread_count, double &time_multithread, double &time_singlethread);
void save_stat(string filename, std::vector<double> &multi,
  std::vector<double> &single, std::vector<uint32_t> &sizes);

processor_info info;

int main(int argc, char **argv) {
  info = get_processor_info();

  const char show_help[] = "Usage:\n"
    "  -h, -help    - Show help\n"
    "  -average <n> - How many tests need to make average value (optional)\n"
    "  -count <n>   - Set max order of matrix.\n"
    "  -graph       - Construct a graph"
    "  -ln          - Use a logarithmic scale (if use steps)\n"
    "  -step <n>    - Set constant step (optional)\n"
    "  -steps <n>   - Set count of steps (optional, mute -step)\n"
    "  -silent      - No log information\n"
    "  -less        - Less log information\n"
    "  -threads <n> - Force set count of threads.";
  const char number_expected[] = "A number is expected.";
  if (argc < 3) {
    std::cout << show_help;
    return 0;
  }

  uint32_t count = 0;
  uint32_t average = 1;
  uint32_t step = 0;
  uint32_t thread_count = info.threads;
  double step_exp = 0.0;
  uint32_t steps = 0;
  string out_filename = "result.html";
  bool use_log = false;
  bool silent = false;
  bool less = false;
  bool build_graph = false;

  // parce params
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help")) {
      std::cout << show_help;
      return 0;
    }

    // размер
    if (!strcmp(argv[i], "-count")) {
      if (i + 1 == argc) {
        std::cout << number_expected;
        return 0;
      }
      else {
        count = atoi(argv[i + 1]);
        if (errno != 0) break;
      }
      continue;
    }

    // Постоянный шаг
    if (!strcmp(argv[i], "-step")) {
      if (i + 1 == argc) {
        std::cout << number_expected;
        return 0;
      }
      else {
        step = atoi(argv[i + 1]);
        if (errno != 0) break;
      }
      continue;
    }

    // Автоматическое подразделение
    if (!strcmp(argv[i], "-steps")) {
      if (i + 1 == argc) {
        std::cout << number_expected;
        return 0;
      }
      else {
        steps = atoi(argv[i + 1]);
        if (errno != 0) break;
      }
      continue;
    }

    // Усреднение
    if (!strcmp(argv[i], "-average")) {
      if (i + 1 == argc) {
        std::cout << number_expected;
        return 0;
      }
      else {
        average = atoi(argv[i + 1]);
        if (errno != 0) break;
      }
      continue;
    }

    // Потоки
    if (!strcmp(argv[i], "-threads")) {
      if (i + 1 == argc) {
        std::cout << number_expected;
        return 0;
      }
      else {
        thread_count = atoi(argv[i + 1]);
        if (errno != 0) break;
      }
      continue;
    }

    // экспоненциальный шаг
    if (!strcmp(argv[i], "-ln")) {
      use_log = true;
      continue;
    }

    // без вывода
    if (!strcmp(argv[i], "-silent")) {
      silent = true;
      continue;
    }

    // сокращенный вывод
    if (!strcmp(argv[i], "-less")) {
      less = true;
      continue;
    }

    // Строить графики
    if (!strcmp(argv[i], "-graph")) {
      build_graph = true;
      continue;
    }
  }

  if (errno == EINVAL) {
    std::cout << number_expected;
    return 0;
  }

  if (count == 0) {
    std::cout << "You need to set max order of matrix using -count <n>";
    return 0;
  }

  if (!silent && !less)
    std::wcout << L"Processor: " << info.name << endl << endl;

  if (thread_count == 0)
    thread_count = info.threads;

  if (steps > 1) {
    if (use_log) {
      step_exp = exp(log((double)count) / steps);
      if (!silent && !less)
        std::cout << "Exponent step set to: " << step_exp << std::endl;
    }
    else {
      step = count / steps;
      if (!silent && !less)
        std::cout << "Step set to: " << step << std::endl;
    }
  }
  else if (steps == 1) {
    step = 0;
  }

  if (steps == 0 && (step <= 0 || step >= count)) {
    double multi_t = 0.0, single_t = 0.0;

    std::cout << "Single test";
    if (average <= 1) {
      if (!silent)
        std::cout << " without avearaging" << std::endl;
      run_test(count, thread_count, multi_t, single_t);
      if (!silent) {
        std::cout << "Time (multi) : " << multi_t * 1000.0 << " ms." << endl;
        std::cout << "Time (single): " << single_t * 1000.0 << " ms." << endl;
      }
    }
    else {
      double t1 = 0, t2 = 0;
      double norm = 1.0 / average;
      if (!silent)
        std::cout << " with avearaging" << std::endl;
      for (uint32_t j = 0; j < average; j++) {
        run_test(count, thread_count, t1, t2);
        if (!silent && !less)
          std::cout << "    Time multi: " << t1 * 1000.0 << " ms., single: " << t2 * 1000.0 << " ms." << endl;
        multi_t += t1 * norm;
        single_t += t2 * norm;
      }
      if (!silent) {
        std::cout << "  Average time (multi) : " << multi_t * 1000.0 << " ms." << endl;
        std::cout << "  Average time (single): " << single_t * 1000.0 << " ms." << endl;
      }
    }
    std::vector<double> list_m = { 0,multi_t * 1000.0 };
    std::vector<double> list_s = { 0,single_t * 1000.0 };
    std::vector<uint32_t> list_c = { 1, count };
    if (build_graph)
      save_stat(out_filename, list_m, list_s, list_c);
  }
  else {
    // group of tests
    uint32_t n = 0;
    if (steps != 0)
      n = steps;
    else
      n = count / step;
    std::vector<uint32_t> count(n);
    std::vector<double> multi_t(n);
    std::vector<double> single_t(n);
    uint32_t size = step;
    double size_exp = step_exp;
    if (use_log) size = (uint32_t)size_exp;

    for (uint32_t i = 0; i < n; i++) {
      count[i] = size;
      multi_t[i] = 0.0;
      single_t[i] = 0.0;
      if (silent) {
        std::cout << "\rProgress: " << i * 100 / n << "%";
      }
      else
        if (!less)
          std::cout << std::endl << "Test # " << i + 1 << " (size: " << size << ")" << std::endl;
      // run tests
      if (average <= 1) {
        run_test(size, thread_count, multi_t[i], single_t[i]);
        multi_t[i] *= 1000.0;
        single_t[i] *= 1000.0;
        if (!silent) {
          std::cout << "Time multi : " << multi_t[i] << " ms." << endl;
          std::cout << "Time single: " << single_t[i] << " ms." << endl;
        }
      }
      else {
        double t1 = 0, t2 = 0;
        double norm = 1.0 / average;
        for (uint32_t j = 0; j < average; j++) {
          run_test(size, thread_count, t1, t2);
          if (!silent && !less)
            std::cout << "    Time multi: " << t1 * 1000.0 << " ms., single: " << t2 * 1000.0 << " ms." << endl;
          multi_t[i] += t1 * norm;
          single_t[i] += t2 * norm;
        }
        multi_t[i] *= 1000.0;
        single_t[i] *= 1000.0;
        if (!silent) {
          std::cout << "  Average time (multi) : " << multi_t[i] << " ms." << endl;
          std::cout << "  Average time (single): " << single_t[i] << " ms." << endl;
        }
      }

      if (use_log) {
        size_exp *= step_exp;
        size = (uint32_t)round(size_exp);
      }
      else
        size += step;
    }
    if (build_graph)
      save_stat(out_filename, multi_t, single_t, count);
  }
  if (build_graph)
    ShellExecuteA((HWND)NULL, "open", out_filename.c_str(), nullptr, nullptr, 0);
  return 0;
}

// стандартная функция умножения
matrix mul(matrix &a, matrix &b) {
  matrix result(b.width(), a.height());
  for (int16_t i = 0; i != a.height(); i++) {
    for (int16_t j = 0; j != a.height(); j++) {
      for (int16_t k = 0; k != a.width(); k++)
        result(i, j) += a(k, j) * b(i, k);
    }
  }
  return result;
}

// умножение многопоточное
matrix mul_th(matrix& a, matrix& b, uint32_t thread_count) {
  matrix result(b.width(), a.height());
  int w = b.width();
  double temp;

#pragma omp parallel for num_threads(thread_count) private(temp)
  for (int i = 0; i < b.width(); i++) {
    for (int j = 0; j < a.height(); j++) {
      temp = 0;
      for (int k = 0; k < a.width(); k++)
        temp += a(k, j) * b(i, k);
      result(i, j) = temp;
    }
  }

  return result;
}

void run_test(size_t size, uint32_t thread_count, double &time_multithread, double &time_singlethread) {
  const double nano_1 = 1.e-9;
  // init
  matrix a(size, size), b(size, size);
  matrix c;
  for (size_t i = 0; i < size; i++)
    for (size_t j = 0; j < size; j++) {
      a(i, j) = 2 * rand() - 1;
      b(i, j) = 2 * rand() - 1;
    }

  chrono::steady_clock::duration ct1;

  // first test - multi thread
  ct1 = chrono::steady_clock::now().time_since_epoch();

  c = mul_th(a, b, thread_count);

  ct1 = chrono::steady_clock::now().time_since_epoch() - ct1;
  time_multithread = ct1.count() * 1.e-9;

  // second test - single thread
  ct1 = chrono::steady_clock::now().time_since_epoch();

  c = mul(a, b);

  ct1 = chrono::steady_clock::now().time_since_epoch() - ct1;
  time_singlethread = ct1.count() * 1.e-9;

}

string Utf16_Ansi(const wstring &s) {
  string result;
  size_t len = s.size() + 1;
  char *c_str = new char[len];
  char ch = '?';
  BOOL b = TRUE;
  WideCharToMultiByte(CP_ACP, 0, s.c_str(), len, c_str, len, &ch, &b);
  result = c_str;
  delete[] c_str;
  return result;
}

void save_stat(string filename,
  std::vector<double> &multi,
  std::vector<double> &single,
  std::vector<uint32_t> &sizes) {
  ofstream fout(filename);
  fout
    << "<!DOCTYPE html>" << std::endl
    << "<html>" << std::endl
    << "  <head>" << std::endl
    << "    <title>results</title>" << std::endl
    << "    <link rel=\"stylesheet\" type=\"text/css\" href=\"result.css\">" << std::endl
    << "    <meta charset = \"utf-8\">" << std::endl
    << "    <script src = \"plot.js\"></script>" << std::endl
    << "    <script>" << std::endl
    << "      data_single = [";
  for (size_t i = 0; i != sizes.size(); i++) {
    fout << "Pair(" << sizes[i] << ',' << single[i] << "), ";
    if (i % 10 == 0)
      fout << endl;
  }
  fout << "];" << std::endl
    << "      data_multi = [";

  for (size_t i = 0; i != sizes.size(); i++) {
    fout << "Pair(" << sizes[i] << ',' << multi[i] << "), ";
    if (i % 10 == 0)
      fout << endl;
  }

  string cpu_caption = Utf16_Ansi(info.name);
  {
    int off;

    if ((off = cpu_caption.find("(R)")) != string::npos)
      cpu_caption.replace(off, 3, "&reg;");

    if ((off = cpu_caption.find("(TM)")) != string::npos)
      cpu_caption.replace(off, 4, "&trade;");
  }
  fout << "];" << std::endl
    << "	window.onload = function(){" << std::endl
    << "		var Canvas = document.getElementById('graph');" << std::endl
    << "		plot2d(Canvas,[data_single,data_multi]);" << std::endl
    << "	}" << std::endl
    << "  </script>" << std::endl
    << "</head>" << std::endl
    << "<body>" << std::endl
    << "  <table>" << std::endl
    << "    <tr><td colspan=\"3\" class=\"caption\">Results:</td></tr>" << std::endl
    << "    <tr>" << std::endl
    << "	  <td rowspan=\"8\">" << std::endl
    << "        <canvas width=\"640\" height=\"512\"  id=\"graph\">" << std::endl
    << "          <p>Your browser not support drawing.</p>" << std::endl
    << "        </canvas>" << std::endl
    << "	  </td>" << std::endl
    << "      <td colspan=\"2\" class=\"cpu_caption\">" << cpu_caption << "</td>" << std::endl
    << "	</tr>" << std::endl
    << "    <tr>" << std::endl
    << "	  <td class=\"par_name\">Frequency:</td>" << std::endl
    << "	  <td>" << info.freqency / 1000.0f << "GHz</td>" << std::endl
    << "	</tr>" << std::endl
    << "    <tr>" << std::endl
    << "	  <td class=\"par_name\">Cores:</td>" << std::endl
    << "	  <td>" << (int)info.cores << "</td>" << std::endl
    << "	</tr>" << std::endl
    << "    <tr>" << std::endl
    << "	  <td class=\"par_name\">Threads:</td>" << std::endl
    << "	  <td>" << (int)info.threads << "</td>" << std::endl
    << "	</tr>" << std::endl
    << "	</tr>" << std::endl
    << "    <tr>" << std::endl
    << "	  <td colspan=\"2\" class=\"cpu_caption\">Legend:</td>" << std::endl
    << "	</tr>" << std::endl

    << "	</tr>" << std::endl
    << "    <tr>" << std::endl
    << "	  <td class=\"par_name\" style=\"background-color: blue;\"></td>" << std::endl
    << "	  <td>single thread</td>" << std::endl
    << "	</tr>" << std::endl

    << "	</tr>" << std::endl
    << "    <tr>" << std::endl
    << "	  <td class=\"par_name\" style=\"background-color: green;\"></td>" << std::endl
    << "	  <td>multi thread</td>" << std::endl
    << "	</tr>" << std::endl

    << "    <tr><td colspan=\"2\"></td></tr>" << std::endl
    << "  </table>" << std::endl
    << "</body>" << std::endl
    << "</html>";
}