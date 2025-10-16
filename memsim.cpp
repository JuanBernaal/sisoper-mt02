// Juan David Bernal Maldonado

#include <bits/stdc++.h>
using namespace std;

class Utils {
public:
    static string toLowerStr(string s) {
        for (char &c : s) c = (char)tolower(c);
        return s;
    }

    static string trim(const string& s) {
        size_t b = s.find_first_not_of(" \t\r\n");
        if (b == string::npos) return "";
        size_t e = s.find_last_not_of(" \t\r\n");
        return s.substr(b, e - b + 1);
    }
};

enum class Policy { First, Best, Worst };

class Segment {
private:
    int start;
    int size;
    bool isFree;
    string processId;
    int requested;

public:
    Segment(int s, int z, bool freeStatus, const string& pid = "", int req = 0)
        : start(s), size(z), isFree(freeStatus), processId(pid), requested(req) {}

    int getStart() const { return start; }
    int getSize() const { return size; }
    bool getIsFree() const { return isFree; }
    string getProcessId() const { return processId; }
    int getRequested() const { return requested; }

    void setSize(int newSize) { size = newSize; }
    void setFree(bool freeStatus) { isFree = freeStatus; }
    void setProcessId(const string& pid) { processId = pid; }
    void setRequested(int req) { requested = req; }
};

class MemoryManager {
private:
    int totalMemory;
    Policy policy;
    vector<Segment> segments;

    void coalesce() {
        if (segments.empty()) return;
        vector<Segment> merged;
        merged.reserve(segments.size());
        merged.push_back(segments[0]);

        for (size_t i = 1; i < segments.size(); ++i) {
            Segment& prev = merged.back();
            Segment& cur = segments[i];
            if (prev.getIsFree() && cur.getIsFree() &&
                prev.getStart() + prev.getSize() == cur.getStart()) {
                prev.setSize(prev.getSize() + cur.getSize());
            } else {
                merged.push_back(cur);
            }
        }
        segments.swap(merged);
    }

    int findFirstFit(int reqSize) const {
        for (size_t i = 0; i < segments.size(); ++i)
            if (segments[i].getIsFree() && segments[i].getSize() >= reqSize)
                return (int)i;
        return -1;
    }

    int findBestFit(int reqSize) const {
        int index = -1, best = INT_MAX;
        for (size_t i = 0; i < segments.size(); ++i)
            if (segments[i].getIsFree() && segments[i].getSize() >= reqSize &&
                segments[i].getSize() < best) {
                best = segments[i].getSize();
                index = (int)i;
            }
        return index;
    }

    int findWorstFit(int reqSize) const {
        int index = -1, worst = -1;
        for (size_t i = 0; i < segments.size(); ++i)
            if (segments[i].getIsFree() && segments[i].getSize() >= reqSize &&
                segments[i].getSize() > worst) {
                worst = segments[i].getSize();
                index = (int)i;
            }
        return index;
    }

    int findHoleIndex(int reqSize) const {
        if (policy == Policy::First) return findFirstFit(reqSize);
        if (policy == Policy::Best) return findBestFit(reqSize);
        return findWorstFit(reqSize);
    }

public:
    MemoryManager(int total, Policy pol) : totalMemory(total), policy(pol) {
        segments.emplace_back(0, totalMemory, true);
    }

    bool allocate(const string& pid, int req, string& msg) {
        if (req <= 0) {
            msg = "Error: tamano solicitado invalido (" + to_string(req) + ").";
            return false;
        }

        int idx = findHoleIndex(req);
        if (idx == -1) {
            int totalFree = 0, largest = 0, freeCount = 0;
            for (const auto& s : segments)
                if (s.getIsFree()) {
                    totalFree += s.getSize();
                    largest = max(largest, s.getSize());
                    freeCount++;
                }
            msg = "Fallo de asignacion para '" + pid + "' (" + to_string(req) + "). "
                  "Libre total=" + to_string(totalFree) +
                  ", mayor hueco=" + to_string(largest) +
                  ", huecos=" + to_string(freeCount) + ".";
            return false;
        }

        Segment hole = segments[idx];
        if (hole.getSize() == req) {
            segments[idx] = Segment(hole.getStart(), req, false, pid, req);
        } else {
            Segment used(hole.getStart(), req, false, pid, req);
            Segment leftover(hole.getStart() + req, hole.getSize() - req, true);
            segments[idx] = used;
            segments.insert(segments.begin() + idx + 1, leftover);
        }

        msg = "OK: Asignado '" + pid + "' (" + to_string(req) + ") en " + to_string(hole.getStart()) + ".";
        return true;
    }

    bool freeProcess(const string& pid, string& msg) {
        bool found = false;
        for (auto& s : segments) {
            if (!s.getIsFree() && s.getProcessId() == pid) {
                s.setFree(true);
                s.setProcessId("");
                s.setRequested(0);
                found = true;
            }
        }
        if (!found) {
            msg = "Aviso: proceso '" + pid + "' no encontrado.";
            return false;
        }

        sort(segments.begin(), segments.end(),
             [](const Segment& a, const Segment& b) { return a.getStart() < b.getStart(); });
        coalesce();
        msg = "OK: liberado '" + pid + "'.";
        return true;
    }

    void showMemory(ostream& out) const {
        for (const auto& s : segments)
            out << (s.getIsFree() ? "[Libre: " + to_string(s.getSize()) + "]"
                                  : "[" + s.getProcessId() + ": " + to_string(s.getSize()) + "]");
        out << "\n";

        int freeTotal = 0, largest = 0, freeCount = 0;
        long long internal = 0;
        for (const auto& s : segments) {
            if (s.getIsFree()) {
                freeTotal += s.getSize();
                largest = max(largest, s.getSize());
                freeCount++;
            } else {
                internal += s.getSize() - s.getRequested();
            }
        }
        int external = freeTotal - largest;
        out << "# Metricas -> FragInterna=" << internal
            << ", FragExterna=" << external
            << ", LibreTotal=" << freeTotal
            << ", MayorHueco=" << largest
            << ", Huecos=" << freeCount << "\n";
    }
};

class ProgramOptions {
private:
    int memSize;
    Policy policy;
    string filePath;

public:
    ProgramOptions() : memSize(100), policy(Policy::First), filePath("") {}

    bool parseArgs(int argc, char** argv) {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mem" && i + 1 < argc) {
                memSize = stoi(argv[++i]);
                if (memSize <= 0) {
                    cerr << "Error: --mem debe ser > 0\n";
                    return false;
                }
            } else if (arg == "--algo" && i + 1 < argc) {
                string val = Utils::toLowerStr(argv[++i]);
                if (val == "first" || val == "firstfit") policy = Policy::First;
                else if (val == "best" || val == "bestfit") policy = Policy::Best;
                else if (val == "worst" || val == "worstfit") policy = Policy::Worst;
                else {
                    cerr << "Error: --algo debe ser first|best|worst\n";
                    return false;
                }
            } else if (arg == "--input" && i + 1 < argc) {
                filePath = argv[++i];
            } else {
                cerr << "Error: argumento no reconocido: " << arg << "\n";
                return false;
            }
        }
        return true;
    }

    int getMemSize() const { return memSize; }
    Policy getPolicy() const { return policy; }
    string getFilePath() const { return filePath; }
};


class CommandProcessor {
public:
    void execute(MemoryManager& mm, istream& in, ostream& out) {
        string line;
        while (getline(in, line)) {
            line = Utils::trim(line);
            if (line.empty() || line[0] == '#') continue;

            istringstream iss(line);
            string op;
            iss >> op;
            if (op.empty()) continue;
            char c = (char)toupper(op[0]);

            if (c == 'A') {
                string pid; int size;
                if (!(iss >> pid >> size)) {
                    out << "Error: formato invalido en 'A <proceso> <tam>'\n";
                    continue;
                }
                string msg;
                bool ok = mm.allocate(pid, size, msg);
                out << (ok ? "" : "WARN: ") << msg << "\n";
            } else if (c == 'L') {
                string pid;
                if (!(iss >> pid)) {
                    out << "Error: formato invalido en 'L <proceso>'\n";
                    continue;
                }
                string msg;
                bool ok = mm.freeProcess(pid, msg);
                out << (ok ? "" : "WARN: ") << msg << "\n";
            } else if (c == 'M') {
                mm.showMemory(out);
            } else {
                out << "Error: operacion desconocida '" << op << "'\n";
            }
        }
    }
};

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ProgramOptions options;
    if (!options.parseArgs(argc, argv)) {
        cerr << "Uso: ./memsim --mem <N> --algo <first|best|worst> [--input archivo]\n";
        return 1;
    }

    MemoryManager mm(options.getMemSize(), options.getPolicy());
    CommandProcessor processor;

    if (options.getFilePath().empty()) {
        processor.execute(mm, cin, cout);
    } else {
        ifstream fin(options.getFilePath());
        if (!fin) {
            cerr << "Error: no se pudo abrir '" << options.getFilePath() << "'\n";
            return 1;
        }
        processor.execute(mm, fin, cout);
    }
    return 0;
}
