-- Example: Using BETWEEN operator with SQL comments
-- This demonstrates the BETWEEN operator for range queries

/* 
 * BETWEEN is inclusive on both ends
 * Syntax: column BETWEEN lower_value AND upper_value
 * Equivalent to: column >= lower_value AND column <= upper_value
 */

-- Select users within age range 25-35
SELECT 
    name,
    age,
    role
FROM './data/users.csv'
WHERE age BETWEEN 25 AND 35  -- Age range filter
ORDER BY age DESC;

-- You can also use BETWEEN with strings (alphabetical order)
-- SELECT name FROM './data/users.csv' 
-- WHERE name BETWEEN 'Alice' AND 'John'
-- ORDER BY name;

-- BETWEEN works with arithmetic expressions too
-- SELECT name, age, age * 2 AS double_age
-- FROM './data/users.csv'
-- WHERE age * 2 BETWEEN 50 AND 70;
