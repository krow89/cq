-- CASE Expression Examples
-- Demonstrates both simple and searched CASE forms

-- Simple CASE: Match exact values
SELECT name, role,
    CASE role
        WHEN 'admin' THEN 'Administrator'
        WHEN 'user' THEN 'Regular User'
        WHEN 'moderator' THEN 'Moderator'
        ELSE 'Unknown Role'
    END AS role_description
FROM 'data/users.csv';

-- Searched CASE: Age categories
SELECT name, age,
    CASE
        WHEN age < 25 THEN 'Young Professional'
        WHEN age >= 25 AND age < 35 THEN 'Early Career'
        WHEN age >= 35 AND age < 50 THEN 'Experienced'
        ELSE 'Senior Professional'
    END AS career_stage
FROM 'data/users.csv'
ORDER BY age;

-- CASE with numeric results (tier system)
SELECT name, age,
    CASE
        WHEN age < 30 THEN 1
        WHEN age < 40 THEN 2
        WHEN age < 50 THEN 3
        ELSE 4
    END AS tier
FROM 'data/users.csv'
ORDER BY tier, age;

-- Nested CASE for complex logic
SELECT name, age, role,
    CASE
        WHEN role = 'admin' THEN
            CASE
                WHEN age < 30 THEN 'Junior Admin'
                ELSE 'Senior Admin'
            END
        WHEN role = 'user' THEN
            CASE
                WHEN age < 25 THEN 'New User'
                WHEN age < 40 THEN 'Regular User'
                ELSE 'Veteran User'
            END
        ELSE 'Moderator'
    END AS user_category
FROM 'data/users.csv';

-- CASE in WHERE clause (filter using conditions)
SELECT name, age, role
FROM 'data/users.csv'
WHERE CASE
    WHEN role = 'admin' THEN 1
    WHEN age > 40 THEN 1
    ELSE 0
END = 1;

-- CASE in ORDER BY (custom sort order)
SELECT name, age, role
FROM 'data/users.csv'
ORDER BY CASE role
    WHEN 'admin' THEN 1
    WHEN 'moderator' THEN 2
    WHEN 'user' THEN 3
    ELSE 4
END, name;

-- CASE without ELSE clause (returns NULL if no match)
SELECT name, age,
    CASE
        WHEN age >= 50 THEN 'Senior'
        WHEN age >= 40 THEN 'Experienced'
    END AS seniority_level
FROM 'data/users.csv';

-- Combining CASE with other functions
SELECT name,
    UPPER(CASE role
        WHEN 'admin' THEN 'Admin'
        WHEN 'moderator' THEN 'Mod'
        ELSE 'User'
    END) AS short_role,
    CONCAT(name, ' (', CASE
        WHEN age < 30 THEN 'Y'
        ELSE 'O'
    END, ')') AS name_with_age_marker
FROM 'data/users.csv'
LIMIT 10;
