#ifndef EMPLOYEESDB_HPP
#define EMPLOYEESDB_HPP

#include <unordered_map>
#include <vector>

enum class Profession
{
    ENGINEER,
    DOCTOR,
    LAWYER
};

struct EmployeeRecord
{
    std::string name;
    Profession position = Profession::ENGINEER;
    int age             = 0;
    int salary          = 0;
    uint64_t id         = 0;

    EmployeeRecord() = default;
    EmployeeRecord(std::string name, Profession position, int age, int salary)
        : name(std::move(name)), position(position), age(age), salary(salary)
    {
    }
};

class EmployeesDb
{
    using Collection = std::vector<EmployeeRecord>;
    using NameLookup = std::unordered_map<std::string, uint64_t>;
    using IdLookup   = std::unordered_map<uint64_t, size_t>;

    Collection mEmployees;
    NameLookup mNameLookup;
    IdLookup mIdLookup;

public:
    using Iterator = Collection::const_iterator;

    EmployeesDb() = default;
    EmployeesDb(std::vector<EmployeeRecord> employees);
    ~EmployeesDb() = default;

    uint64_t insert(EmployeeRecord data);
    void remove(const std::string& name);
    void remove(uint64_t id);
    Iterator find(uint64_t id) const;
    Iterator find(const std::string& name) const;
    size_t size() const;
    Iterator begin() const;
    Iterator end() const;

private:
    void generateIdLookup();
    void generateNameLookup();
};

std::pair<EmployeesDb::Iterator, EmployeesDb::Iterator> range(const EmployeesDb& db,
                                                              Profession position);

std::pair<EmployeeRecord, EmployeeRecord> minMaxSalaryPerPosition(const EmployeesDb& db,
                                                                  Profession position);

int avgSalaryPerPosition(const EmployeesDb& db, Profession position);

int medianSalaryPerPosition(const EmployeesDb& db, Profession position);

std::vector<EmployeeRecord>
topNSalariesPerPosition(const EmployeesDb& db, Profession position, int n);

int avgSalaryPerAgeRange(const EmployeesDb& db, std::pair<int, int> ageRange);

std::string dumpEmployeeRecord(const EmployeeRecord& r);

std::string dumpEmployeeDb(const EmployeesDb& db);

#endif // EMPLOYEESDB_HPP
