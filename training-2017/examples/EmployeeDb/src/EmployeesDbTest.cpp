#include "EmployeesDb.hpp"
#include <algorithm>
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <random>

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

bool
checkIsPartitioned(const EmployeesDb& db, Profession p)
{
    auto it = std::lower_bound(
        db.begin(), db.end(), p,
        [](const EmployeeRecord& e, Profession pos) { return e.position < pos; });

    return std::is_partitioned(it, db.end(),
                               [p](const EmployeeRecord& e) { return e.position == p; });
}

TEST(TestDb, FindByName)
{
    auto testVec = generateRandomEmployees(20);
    EmployeesDb db{testVec};
    for (const auto& e : testVec)
    {
        EXPECT_FALSE(db.find(e.name) == db.end());
    }
}

TEST(TestDb, Insert)
{
    EmployeesDb db;
    std::vector<uint64_t> ids;
    ids.push_back(db.insert({"John", Profession::DOCTOR, 40, 5000}));
    ids.push_back(db.insert({"Franek", Profession::LAWYER, 40, 5000}));
    ids.push_back(db.insert({"Adam", Profession::DOCTOR, 40, 5000}));
    ids.push_back(db.insert({"Yoda", Profession::LAWYER, 40, 5000}));
    ids.push_back(db.insert({"Windu", Profession::ENGINEER, 40, 5000}));
    ids.push_back(db.insert({"Luke", Profession::ENGINEER, 40, 5000}));
    ids.push_back(db.insert({"Rambo", Profession::DOCTOR, 40, 5000}));

    for (const auto& id : ids)
    {
        EXPECT_FALSE(db.find(id) == db.end());
    }

    EXPECT_FALSE(db.find("Franek") == db.end());
    EXPECT_FALSE(db.find("Rambo") == db.end());

    EXPECT_TRUE(checkIsPartitioned(db, Profession::ENGINEER));
    EXPECT_TRUE(checkIsPartitioned(db, Profession::DOCTOR));
    EXPECT_TRUE(checkIsPartitioned(db, Profession::LAWYER));
}

TEST(TestDb, IsPartitioned)
{
    auto testVec = generateRandomEmployees(50);
    EmployeesDb db{testVec};
    EXPECT_TRUE(checkIsPartitioned(db, Profession::ENGINEER));
    EXPECT_TRUE(checkIsPartitioned(db, Profession::DOCTOR));
    EXPECT_TRUE(checkIsPartitioned(db, Profession::LAWYER));
}

TEST(TestDb, GetProfessionRange)
{
    auto testVec = generateRandomEmployees(50);
    EmployeesDb db{testVec};
    auto r = range(db, Profession::DOCTOR);
    std::for_each(r.first, r.second, [](const EmployeeRecord& e) {
        EXPECT_EQ(e.position, Profession::DOCTOR);
    });
    std::for_each(r.second, db.end(), [](const EmployeeRecord& e) {
        EXPECT_NE(e.position, Profession::DOCTOR);
    });

    std::for_each(db.begin(), r.first, [](const EmployeeRecord& e) {
        EXPECT_NE(e.position, Profession::DOCTOR);
    });
    std::for_each(r.second, db.end(), [](const EmployeeRecord& e) {
        EXPECT_NE(e.position, Profession::DOCTOR);
    });
}

class StatsTest : public ::testing::Test
{
public:
    std::vector<EmployeeRecord> setupVec;
    int avgSalaryEng = 0;
    int avgSalaryMgr = 0;
    int avgSalaryDir = 0;
    int medSalaryEng = 0;
    int medSalaryMgr = 0;
    int medSalaryDir = 0;

    std::vector<std::string> top3SalariesEng;

    void
    SetUp() override
    {
        setupVec.push_back({"John", Profession::DOCTOR, 25, 1000});
        setupVec.push_back({"Franek", Profession::LAWYER, 30, 2000});
        setupVec.push_back({"Adam", Profession::DOCTOR, 23, 3400});
        setupVec.push_back({"Yoda", Profession::LAWYER, 50, 5000});
        setupVec.push_back({"Windu", Profession::ENGINEER, 60, 3000});
        setupVec.push_back({"Luke", Profession::ENGINEER, 44, 9000});
        setupVec.push_back({"Rambo", Profession::DOCTOR, 36, 8000});
        setupVec.push_back({"Lucky Luke", Profession::LAWYER, 51, 7000});
        setupVec.push_back({"Micky Mouse", Profession::ENGINEER, 41, 3000});
        setupVec.push_back({"Jerry", Profession::ENGINEER, 31, 7000});
        setupVec.push_back({"Bronek", Profession::ENGINEER, 41, 4000});
        setupVec.push_back({"Enek", Profession::ENGINEER, 61, 1000});

        avgSalaryEng = 4500;
        avgSalaryMgr = 4133;
        avgSalaryDir = 4666;

        medSalaryEng = 4000;
        medSalaryMgr = 3400;
        medSalaryDir = 5000;

        top3SalariesEng = {"Luke", "Jerry", "Bronek"};
    }
};

TEST_F(StatsTest, AvgSalary)
{
    EmployeesDb db(setupVec);
    EXPECT_EQ(avgSalaryEng, avgSalaryPerPosition(db, Profession::ENGINEER));
    EXPECT_EQ(avgSalaryMgr, avgSalaryPerPosition(db, Profession::DOCTOR));
    EXPECT_EQ(avgSalaryDir, avgSalaryPerPosition(db, Profession::LAWYER));
}

TEST_F(StatsTest, MedianSalary)
{
    EmployeesDb db(setupVec);
    EXPECT_EQ(medSalaryEng, medianSalaryPerPosition(db, Profession::ENGINEER));
    EXPECT_EQ(medSalaryMgr, medianSalaryPerPosition(db, Profession::DOCTOR));
    EXPECT_EQ(medSalaryDir, medianSalaryPerPosition(db, Profession::LAWYER));
}

TEST_F(StatsTest, TopNSalary)
{
    EmployeesDb db(setupVec);
    auto employees = topNSalariesPerPosition(db, Profession::ENGINEER, 3);
    std::vector<std::string> names(3);
    std::transform(employees.begin(), employees.end(), names.begin(),
                   [](const EmployeeRecord& e) { return e.name; });

    EXPECT_EQ(top3SalariesEng, names);
}

TEST(TestDb, Misc)
{
    enum Color
    {
        RED,
        GREEN,
        BLUE
    };

    std::array<const char*, 3> names = {"red", "green", "blue"};

    std::vector<Color> colors = {BLUE,  RED,   BLUE, GREEN, RED,   RED,   BLUE, GREEN,
                                 GREEN, GREEN, BLUE, RED,   GREEN, GREEN, BLUE};
    auto it =
        std::partition(colors.begin(), colors.end(), [](Color c) { return c == BLUE; });
    std::for_each(it, colors.end(), [&names](Color c) { std::cout << names[c] << ' '; });
    std::cout << std::endl;

    std::vector<int> v = {4, 12, 9, 3, 7, 13, 5, 8, 4, 5, 11};
    std::nth_element(v.begin(), v.end() - 5, v.end());
    for (auto& i : v)
        std::cout << i << ' ';
    std::cout << std::endl;
}

int
main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
