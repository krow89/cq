-- Example: JOIN operations
-- Demonstrates different types of joins between CSV files

/*
 * JOIN Types:
 * - INNER JOIN: Returns only matching rows from both tables
 * - LEFT JOIN: All rows from left table + matching from right
 * - RIGHT JOIN: All rows from right table + matching from left
 * - FULL JOIN: All rows from both tables
 */

-- INNER JOIN example: Users with their email addresses
SELECT 
    u.name,
    u.role,
    u.age,
    e.email
FROM './data/users.csv' AS u
INNER JOIN './data/emails.csv' AS e ON u.id = e.id
WHERE u.active = 1  -- Only active users
ORDER BY u.name;

-- LEFT JOIN example: All users, with emails where available
-- SELECT 
--     u.name,
--     u.role,
--     e.email
-- FROM './data/users.csv' AS u
-- LEFT JOIN './data/emails.csv' AS e ON u.id = e.id
-- ORDER BY u.name;

-- Multiple JOINs example
-- SELECT 
--     u.name,
--     u.role,
--     e.email,
--     c.city_name
-- FROM './data/users.csv' AS u
-- LEFT JOIN './data/emails.csv' AS e ON u.id = e.id
-- LEFT JOIN './data/cities.csv' AS c ON u.city_id = c.id
-- WHERE u.age BETWEEN 25 AND 40
-- ORDER BY c.city_name, u.name;

-- JOIN with aggregation
-- SELECT 
--     u.role,
--     COUNT(*) AS user_count,
--     COUNT(e.email) AS email_count
-- FROM './data/users.csv' AS u
-- LEFT JOIN './data/emails.csv' AS e ON u.id = e.id
-- GROUP BY u.role
-- ORDER BY user_count DESC;
