#include "EmployeesDb.hpp"
#include <algorithm>
#include <iostream>
#include <random>

namespace {
uint64_t
generateId()
{
    static uint64_t id = 1;
    return id++;
}
}

EmployeesDb::EmployeesDb(std::vector<EmployeeRecord> employees)
    : mEmployees(std::move(employees))
{
    // Collection internally partitioned by Profession
    auto it =
        std::partition(mEmployees.begin(), mEmployees.end(), [](const EmployeeRecord& e) {
            return e.position == Profession::ENGINEER;
        });
    std::partition(it, mEmployees.end(), [](const EmployeeRecord& e) {
        return e.position == Profession::DOCTOR;

    });

    generateNameLookup();
    generateIdLookup();
}

uint64_t
EmployeesDb::insert(EmployeeRecord data)
{
    auto it = mNameLookup.find(data.name);
    if (it == mNameLookup.end())
    {
        auto it2 = std::upper_bound(
            mEmployees.begin(), mEmployees.end(), data.position,
            [](Profession pos, const EmployeeRecord& e) { return pos < e.position; });
        auto id = generateId();
        data.id = id;
        mNameLookup.emplace(data.name, id);
        mEmployees.insert(it2, std::move(data));
        generateIdLookup();
        return id;
    }
    else
    {
        auto& rec = mEmployees[it->second];
        auto id   = rec.id;
        rec       = std::move(data);
        rec.id    = id;
        return id;
    }
}

void
EmployeesDb::remove(uint64_t id)
{
    auto it = mIdLookup.find(id);
    if (it != mIdLookup.end())
    {
        auto index = it->second;
        auto& name = mEmployees[index].name;
        mNameLookup.erase(name);
        mEmployees.erase(mEmployees.begin() + index);
        generateIdLookup();
    }
}

void
EmployeesDb::remove(const std::string& name)
{
    auto it = mNameLookup.find(name);
    if (it != mNameLookup.end())
    {
        remove(it->second);
    }
}

EmployeesDb::Iterator
EmployeesDb::find(uint64_t id) const
{
    auto it = mIdLookup.find(id);
    if (it != mIdLookup.end())
    {
        return mEmployees.begin() + it->second;
    }
    return end();
}

EmployeesDb::Iterator
EmployeesDb::find(const std::string& name) const
{
    auto it1 = mNameLookup.find(name);
    if (it1 != mNameLookup.end())
    {
        auto it2 = mIdLookup.find(it1->second);
        if (it2 != mIdLookup.end())
        {
            return mEmployees.begin() + it2->second;
        }
    }
    return end();
}

size_t
EmployeesDb::size() const
{
    return mEmployees.size();
}

EmployeesDb::Iterator
EmployeesDb::begin() const
{
    return mEmployees.begin();
}

EmployeesDb::Iterator
EmployeesDb::end() const
{
    return mEmployees.end();
}

std::pair<EmployeesDb::Iterator, EmployeesDb::Iterator>
range(const EmployeesDb& db, Profession position)
{
    auto compPos1 = [](const EmployeeRecord& e, Profession pos) {
        return e.position < pos;
    };

    auto compPos2 = [](Profession pos, const EmployeeRecord& e) {
        return pos < e.position;
    };

    auto begin = std::lower_bound(db.begin(), db.end(), position, compPos1);
    auto end   = std::upper_bound(db.begin(), db.end(), position, compPos2);
    return {begin, end};
}

std::pair<EmployeeRecord, EmployeeRecord>
minMaxSalaryPerPosition(const EmployeesDb& db, Profession position)
{
    auto r      = range(db, position);
    auto minMax = std::minmax_element(
        r.first, r.second, [](const EmployeeRecord& e1, const EmployeeRecord& e2) {
            return e1.salary < e2.salary;
        });
    return {*minMax.first, *minMax.second};
}

int
avgSalaryPerPosition(const EmployeesDb& db, Profession position)
{
    auto r = range(db, position);
    auto totalSalary =
        std::accumulate(r.first, r.second, uint64_t{0},
                        [](uint64_t s, const EmployeeRecord& e) { return s + e.salary; });
    auto noOfEmployees = std::distance(r.first, r.second);
    return totalSalary / noOfEmployees;
}

int
medianSalaryPerPosition(const EmployeesDb& db, Profession position)
{
    auto r             = range(db, position);
    auto noOfEmployees = std::distance(r.first, r.second);
    std::vector<int> salaries(noOfEmployees);
    std::transform(r.first, r.second, salaries.begin(),
                   [](const EmployeeRecord& e) { return e.salary; });

    std::nth_element(salaries.begin(), salaries.begin() + salaries.size() / 2,
                     salaries.end());
    return salaries[salaries.size() / 2];
}

std::vector<EmployeeRecord>
topNSalariesPerPosition(const EmployeesDb& db, Profession position, int n)
{
    auto r             = range(db, position);
    auto noOfEmployees = std::distance(r.first, r.second);

    struct Helper
    {
        uint64_t id;
        int salary;
    };

    std::vector<Helper> data(noOfEmployees);
    std::transform(r.first, r.second, data.begin(), [](const EmployeeRecord& e) {
        return Helper{e.id, e.salary};
    });

    auto fSalaryComp = [](const Helper& e1, const Helper& e2) {
        return e1.salary < e2.salary;
    };

    std::nth_element(data.begin(), data.end() - n, data.end(), fSalaryComp);
    std::sort(data.end() - n, data.end(), fSalaryComp);

    std::vector<EmployeeRecord> employeesTopN(n);
    std::transform(data.rbegin(), data.rbegin() + n, employeesTopN.begin(),
                   [&db](const Helper& e) { return *db.find(e.id); });
    return employeesTopN;
}

int
avgSalaryPerAgeRange(const EmployeesDb& db, std::pair<int, int> ageRange)
{
    struct Helper
    {
        int age;
        int salary;
    };

    std::vector<Helper> data(db.size());
    std::transform(db.begin(), db.end(), data.begin(), [](const EmployeeRecord& e) {
        return Helper{e.age, e.salary};
    });

    auto it = std::partition(data.begin(), data.end(), [&ageRange](const Helper& e) {
        return e.age >= ageRange.first && e.age <= ageRange.second;
    });

    auto totalSalary =
        std::accumulate(data.begin(), it, uint64_t{0},
                        [](uint64_t s, const Helper& e) { return s + e.salary; });
    auto noOfEmployees = std::distance(data.begin(), it);
    return totalSalary / noOfEmployees;
}

void
EmployeesDb::generateIdLookup()
{
    mIdLookup.clear();
    for (auto i = 0; i < mEmployees.size(); i++)
    {
        mIdLookup.emplace(mEmployees[i].id, i);
    }
}

void
EmployeesDb::generateNameLookup()
{
    mNameLookup.clear();
    for (auto& e : mEmployees)
    {
        e.id = generateId();
        mNameLookup.emplace(e.name, e.id);
    }
}

std::string
dumpEmployeeRecord(const EmployeeRecord& r)
{
    auto str = std::string{"Name="} + r.name + ", Position=" +
               std::to_string((int)r.position) + ", Age=" + std::to_string(r.age) +
               ", Salary=" + std::to_string(r.salary);
    return str;
}

std::string
dumpEmployeeDb(const EmployeesDb& db)
{
    return std::accumulate(db.begin(), db.end(), std::string{},
                           [](const std::string& s, const EmployeeRecord& e) {
                               return s + dumpEmployeeRecord(e) + '\n';
                           });
}
