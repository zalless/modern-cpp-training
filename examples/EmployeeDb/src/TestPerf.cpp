#include "EmployeesDb.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <random>
#include <sstream>

std::random_device rd;

std::vector<EmployeeRecord>
generateRandomEmployees(size_t n)
{
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> distAge(25, 60);
    std::uniform_int_distribution<int> distSalary(1000, 10000);
    std::uniform_int_distribution<int> distPosition(0, 2);

    auto fMakeRandomEmployee = [&]() {
        static uint32_t id = 1;
        auto name          = std::string("Employee_").append(std::to_string(id++));
        auto position      = static_cast<Profession>(distPosition(re));
        auto age           = distAge(re);
        auto salary        = distSalary(re);
        return EmployeeRecord{std::move(name), position, age, salary};
    };

    std::vector<EmployeeRecord> employees(n);
    std::generate(employees.begin(), employees.end(), fMakeRandomEmployee);
    return employees;
}

std::pair<double, std::pair<int, int>>
timeMinMaxSalary(const EmployeesDb& db)
{
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    std::chrono::duration<double> elapsed_seconds;
    start           = std::chrono::high_resolution_clock::now();
    auto minMax     = minMaxSalaryPerPosition(db, Profession::DOCTOR);
    end             = std::chrono::high_resolution_clock::now();
    elapsed_seconds = end - start;
    return {elapsed_seconds.count(), {minMax.first.salary, minMax.second.salary}};
}

template <typename F, typename... Args>
double
measureTime(F&& f, Args&&... args)
{
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    start    = std::chrono::high_resolution_clock::now();
    auto val = f(std::forward<Args>(args)...);
    end      = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    return elapsed_seconds.count();
}

int
main(int argc, char* argv[])
{
    std::ostringstream oss;
    oss << "#n            minMaxSalary            avgSalaray            medianSalary     "
           "    "
           "top10Salary              avgSalaryAge\n";

    for (auto n = 100; n <= 10000000; n = n * 10)
    {
        auto testVec = generateRandomEmployees(n);
        EmployeesDb db(testVec);
        auto t1 = measureTime(minMaxSalaryPerPosition, db, Profession::DOCTOR);
        auto t2 = measureTime(avgSalaryPerPosition, db, Profession::DOCTOR);
        auto t3 = measureTime(medianSalaryPerPosition, db, Profession::DOCTOR);
        auto t4 = measureTime(topNSalariesPerPosition, db, Profession::DOCTOR, 10);
        auto t5 = measureTime(avgSalaryPerAgeRange, db, std::make_pair(45, 55));

        oss << n << "                " << t1 << "              " << t2 << "             "
            << t3 << "              " << t4 << "              " << t5 << "\n";
    }
    std::cout << oss.str();
    std::ofstream f(argv[1]);
    f << oss.str();
    f.close();
}
