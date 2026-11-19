#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
class FleetError : public std::runtime_error {
public:
    explicit FleetError(const std::string& msg) : std::runtime_error(msg) {}
};
class InspectionRejected : public FleetError {
public:
    explicit InspectionRejected(const std::string& name)
        : FleetError("inspection rejected " + name) {}
};
class CommandError : public FleetError {
public:
    explicit CommandError(const std::string& msg) : FleetError(msg) {}
};
class InventoryItem {
public:
    virtual ~InventoryItem() = default;
    virtual std::string inventoryId() const = 0;
    virtual std::string title() const = 0;
    virtual std::string describe() const = 0;
    bool operator<(const InventoryItem& other) const {
        return inventoryId() < other.inventoryId();
    }
};
class EnergyConsumer {
public:
    virtual ~EnergyConsumer() = default;
    virtual double dailyEnergy() const = 0;
};
struct ModelSpec {
    std::string modelName;
    explicit ModelSpec(std::string n) : modelName(std::move(n)) {}
};
inline std::ostream& operator<<(std::ostream& os, const InventoryItem& item) {
    os << item.title() << " " << item.inventoryId();
    return os;
}
class Transport : public InventoryItem {
protected:
    std::string id_;
    std::string name_;
    int noviceScore_;
    std::shared_ptr<ModelSpec> spec_;

public:
    Transport(std::string id, std::string name, int score,
              std::shared_ptr<ModelSpec> spec)
        : id_(std::move(id)), name_(std::move(name)),
          noviceScore_(score), spec_(std::move(spec)) {}
    std::string inventoryId() const override { return id_; }
    std::string title() const override { return name_; }
    int noviceScore() const { return noviceScore_; }
    bool suitableForNovice() const { return noviceScore_ >= 6; }
    std::string describe() const override {
        std::ostringstream os;
        os << name_ << " (" << id_ << ")";
        return os.str();
    }
};

class ElectricScooter : public Transport, public EnergyConsumer {
    double energy_;
public:
    ElectricScooter(std::string id, std::string name, double energy, int score,
                    std::shared_ptr<ModelSpec> spec)
        : Transport(std::move(id), std::move(name), score, std::move(spec)),
          energy_(energy) {}

    double dailyEnergy() const override { return energy_; }

    std::string describe() const override {
        return Transport::describe() + " scooter";
    }
};
class ElectricBike : public Transport, public EnergyConsumer {
    double energy_;
public:
    ElectricBike(std::string id, std::string name, double energy, int score,
                 std::shared_ptr<ModelSpec> spec)
        : Transport(std::move(id), std::move(name), score, std::move(spec)),
          energy_(energy) {}

    double dailyEnergy() const override { return energy_; }
    std::string describe() const override {
        return Transport::describe() + " ebike";
    }
};
class Bike : public Transport {
public:
    Bike(std::string id, std::string name, int score,
         std::shared_ptr<ModelSpec> spec)
        : Transport(std::move(id), std::move(name), score, std::move(spec)) {}

    std::string describe() const override {
        return Transport::describe() + " bike";
    }
};
class Helmet : public InventoryItem {
    std::string id_, name_;
public:
    Helmet(std::string id, std::string name)
        : id_(std::move(id)), name_(std::move(name)) {}
    std::string inventoryId() const override { return id_; }
    std::string title() const override { return name_; }
    std::string describe() const override {
        return name_ + " (" + id_ + ") helmet";
    }
};
class Dock : public InventoryItem, public EnergyConsumer {
    std::string id_, name_;
    double energy_;
public:
    Dock(std::string id, std::string name, double energy)
        : id_(std::move(id)), name_(std::move(name)), energy_(energy) {}
    std::string inventoryId() const override { return id_; }
    std::string title() const override { return name_; }
    double dailyEnergy() const override { return energy_; }
    std::string describe() const override {
        return name_ + " (" + id_ + ") dock";
    }
};
template <typename T, int Cap>
class Registry {
    std::vector<T> items_;
public:
    bool add(const T& item) {
        if (static_cast<int>(items_.size()) >= Cap) return false;
        items_.push_back(item);
        return true;
    }
    int size() const { return static_cast<int>(items_.size()); }
    const T& operator[](int i) const { return items_.at(i); }
    T& operator[](int i) { return items_.at(i); }

    template <typename U, int C>
    friend std::ostream& operator<<(std::ostream& os, const Registry<U, C>& r);
};
template <typename U, int Cap>
std::ostream& operator<<(std::ostream& os, const Registry<U, Cap>& r) {
    for (const auto& item : r.items_) os << item << "\n";
    return os;
}
template <int Cap>
class Registry<double, Cap> {
    std::vector<double> items_;
public:
    bool add(double v) {
        if (static_cast<int>(items_.size()) >= Cap) return false;
        items_.push_back(v);
        return true;
    }
    double sum() const {
        double s = 0.0;
        for (double v : items_) s += v;
        return s;
    }
    int size() const { return static_cast<int>(items_.size()); }
};
class ServiceCenter {
public:
    static bool inspect(const std::string& name) {
        std::string lower;
        for (char c : name)
            lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lower.find("broken") != std::string::npos)
            throw InspectionRejected(name);
        return true;
    }
};
class Fleet {
    std::vector<std::unique_ptr<InventoryItem>> items_;
    std::shared_ptr<InventoryItem> focusOwner_;
    std::weak_ptr<InventoryItem> focus_;
    Registry<double, 1000> energyLog_;
public:
    void addItem(std::unique_ptr<InventoryItem> item) {
        if (auto* ec = dynamic_cast<EnergyConsumer*>(item.get()))
            energyLog_.add(ec->dailyEnergy());
        items_.push_back(std::move(item));
    }
    double totalEnergy() const { return energyLog_.sum(); }
    void listNovice() const {
        for (const auto& it : items_) {
            if (auto* tr = dynamic_cast<const Transport*>(it.get()))
                if (tr->suitableForNovice())
                    std::cout << tr->title() << " " << tr->inventoryId() << "\n";
        }
    }
    void printAll() const {
        std::vector<const InventoryItem*> refs;
        for (const auto& it : items_) refs.push_back(it.get());
        std::sort(refs.begin(), refs.end(),
                  [](const InventoryItem* a, const InventoryItem* b) { return *a < *b; });
        for (const auto* r : refs) std::cout << *r << "\n";
    }
    void setFocus(const std::string& id) {
        for (const auto& it : items_) {
            if (it->inventoryId() == id) {
                focusOwner_ = std::shared_ptr<InventoryItem>(it.get(), [](InventoryItem*){});
                focus_ = focusOwner_;
                return;
            }
        }
        throw CommandError("focus: id not found " + id);
    }
    void showFocus() const {
        if (auto sp = focus_.lock()) std::cout << *sp << "\n";
        else std::cout << "expired\n";
    }
    void remove(const std::string& id) {
        auto it = std::find_if(items_.begin(), items_.end(),
            [&](const std::unique_ptr<InventoryItem>& p){ return p->inventoryId() == id; });
        if (it == items_.end()) throw CommandError("remove: id not found " + id);
        if (focusOwner_ && focusOwner_.get() == it->get()) focusOwner_.reset();
        items_.erase(it);
    }
    int count() const { return static_cast<int>(items_.size()); }
};
static int parseInt(const std::string& s, const std::string& what) {
    try { return std::stoi(s); }
    catch (...) { throw CommandError("invalid " + what + ": " + s); }
}
static double parseDouble(const std::string& s, const std::string& what) {
    try { return std::stod(s); }
    catch (...) { throw CommandError("invalid " + what + ": " + s); }
}
static int parseScore(const std::string& s) {
    int v = parseInt(s, "score");
    if (v < 1 || v > 10) throw CommandError("score out of range 1..10: " + s);
    return v;
}
static double parseEnergy(const std::string& s) {
    double v = parseDouble(s, "energy");
    if (v < 0.0 || std::isnan(v)) throw CommandError("negative energy: " + s);
    return v;
}
static void requireId(const std::string& id) {
    if (id.empty()) throw CommandError("empty id");
}
static void processCommand(Fleet& fleet, std::istringstream& iss, bool& quit) {
    std::string cmd;
    iss >> cmd;
    if (cmd.empty()) return;
    if (cmd == "quit") { quit = true; return; }
    if (cmd == "add") {
        std::string kind; iss >> kind;
        if (kind == "scooter") {
            std::string id, name, es, ss;
            if (!(iss >> id >> name >> es >> ss))
                throw CommandError("usage: add scooter id name energy score");
            requireId(id);
            double e = parseEnergy(es);
            int sc = parseScore(ss);
            ServiceCenter::inspect(name);
            fleet.addItem(std::make_unique<ElectricScooter>(
                id, name, e, sc, std::make_shared<ModelSpec>(name)));
        } else if (kind == "bike") {
            std::string id, name, ss;
            if (!(iss >> id >> name >> ss))
                throw CommandError("usage: add bike id name score");
            requireId(id);
            int sc = parseScore(ss);
            ServiceCenter::inspect(name);
            fleet.addItem(std::make_unique<Bike>(
                id, name, sc, std::make_shared<ModelSpec>(name)));
        } else if (kind == "ebike") {
            std::string id, name, es, ss;
            if (!(iss >> id >> name >> es >> ss))
                throw CommandError("usage: add ebike id name energy score");
            requireId(id);
            double e = parseEnergy(es);
            int sc = parseScore(ss);
            ServiceCenter::inspect(name);
            fleet.addItem(std::make_unique<ElectricBike>(
                id, name, e, sc, std::make_shared<ModelSpec>(name)));
        } else if (kind == "helmet") {
            std::string id, name;
            if (!(iss >> id >> name)) throw CommandError("usage: add helmet id name");
            requireId(id);
            fleet.addItem(std::make_unique<Helmet>(id, name));
        } else if (kind == "dock") {
            std::string id, name, es;
            if (!(iss >> id >> name >> es)) throw CommandError("usage: add dock id name energy");
            requireId(id);
            double e = parseEnergy(es);
            fleet.addItem(std::make_unique<Dock>(id, name, e));
        } else {
            throw CommandError("unknown add kind: " + kind);
        }
        return;
    }
    if (cmd == "energy") {
        std::cout << std::fixed << std::setprecision(1) << fleet.totalEnergy() << "\n";
        return;
    }
    if (cmd == "novice")     { fleet.listNovice(); return; }
    if (cmd == "list")       { fleet.printAll(); return;}
    if (cmd == "focus")      { std::string id; iss >> id; fleet.setFocus(id); return; }
    if (cmd == "show_focus") { fleet.showFocus(); return; }
    if (cmd == "remove")     { std::string id; iss >> id; fleet.remove(id); return; }
    if (cmd == "count")      { std::cout <<                                                                                        fleet.count() << "\n"; return; }
    throw CommandError("unknown command: " + cmd);
}
int main() {
    Fleet fleet;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        bool quit = false;
        try {
            processCommand(fleet, iss, quit);
        } catch (const FleetError& e) {
            std::cout << "error: " << e.what() << "\n";
        } catch (const std::exception& e) {
            std::cout << "error: " << e.what() << "\n";
        }
        if (quit) break;
    }
    return 0;
}