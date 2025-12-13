-- Example: Data Manipulation Language (DML) operations
-- Shows how to INSERT, UPDATE, and DELETE data in CSV files

/* 
 * WARNING: These operations modify CSV files directly!
 * Make backups before running DML operations on important data.
 */

-- ===== INSERT EXAMPLES =====

-- Insert with all columns specified
-- INSERT INTO './data/users.csv' (id, name, age, role, height, active, email, city)
-- VALUES (100, 'Mario', 35, 'developer', 180.0, 1, 'mario@example.com', 'Rome');

-- Insert with values only (must match column order)
-- INSERT INTO './data/users.csv' 
-- VALUES (101, 'Luigi', 32, 'developer', 175.5, 1, 'luigi@example.com', 'Milan');

-- ===== UPDATE EXAMPLES =====

-- Update single column
-- UPDATE './data/users.csv' 
-- SET active = 0 
-- WHERE age < 18;

-- Update multiple columns with condition
-- UPDATE './data/users.csv'
-- SET role = 'senior', active = 1
-- WHERE age > 40 AND role = 'user';

-- Update all rows (use with caution!)
-- UPDATE './data/users.csv'
-- SET active = 1;

-- ===== DELETE EXAMPLES =====

-- Delete with simple condition
-- DELETE FROM './data/users.csv' 
-- WHERE active = 0;

-- Delete with complex condition
-- DELETE FROM './data/users.csv'
-- WHERE age < 18 OR role = 'guest';

-- Note: DELETE always requires a WHERE clause for safety
-- This prevents accidentally deleting all data

/* 
 * To run these examples:
 * 1. Uncomment the desired operation
 * 2. Run: cq -f assets/example_dml.sql
 * 3. Verify changes in the CSV file
 */
