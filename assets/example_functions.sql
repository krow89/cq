-- Example: String and Math functions
-- Demonstrates scalar functions in SELECT and WHERE clauses

/* 
 * String Functions:
 * - UPPER(), LOWER() - Case conversion
 * - CONCAT() - String concatenation
 * - SUBSTRING() - Extract substring
 * - LENGTH() - String length
 * - REPLACE() - Replace text
 * 
 * Math Functions:
 * - ROUND(), CEIL(), FLOOR() - Rounding
 * - ABS() - Absolute value
 * - POWER() - Exponentiation
 * - SQRT() - Square root
 * - MOD() - Modulo
 */

-- String manipulation example
SELECT 
    name,
    UPPER(name) AS name_upper,
    LENGTH(name) AS name_length,
    CONCAT(name, ' - ', role) AS full_info,
    SUBSTRING(name, 1, 3) AS short_name
FROM './data/users.csv'
WHERE LENGTH(name) > 4  -- Names longer than 4 characters
ORDER BY name;

-- Math functions example
-- SELECT 
--     name,
--     age,
--     ROUND(height / 100, 2) AS height_meters,
--     POWER(age, 2) AS age_squared,
--     MOD(age, 10) AS age_last_digit,
--     CEIL(height) AS height_ceiling,
--     FLOOR(height) AS height_floor
-- FROM './data/users.csv'
-- WHERE age BETWEEN 25 AND 40
-- ORDER BY age;

-- Nested functions example
-- SELECT 
--     UPPER(CONCAT(name, ' (', role, ')')) AS formatted_name,
--     LENGTH(UPPER(name)) AS upper_length
-- FROM './data/users.csv'
-- WHERE SUBSTRING(UPPER(name), 1, 1) = 'A';  -- Names starting with 'A'
