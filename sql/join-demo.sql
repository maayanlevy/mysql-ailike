-- Synthetic products for the README's JOIN example.
-- Load into a fresh database. Categories: 1 = drinkware, 2 = headphones.
CREATE TABLE supplier_products (
  id INT PRIMARY KEY,
  category_id INT NOT NULL,
  description VARCHAR(255) NOT NULL
) DEFAULT CHARSET = utf8mb4;

CREATE TABLE catalog_products (
  id INT PRIMARY KEY,
  category_id INT NOT NULL,
  description VARCHAR(255) NOT NULL
) DEFAULT CHARSET = utf8mb4;

INSERT INTO supplier_products (id, category_id, description) VALUES
  (1, 1, '500 ml stainless steel vacuum-insulated water bottle for hot or cold drinks'),
  (2, 1, '1 litre glass carafe for serving cold water at the table'),
  (3, 2, 'Wireless over-ear headphones with active noise cancellation');

INSERT INTO catalog_products (id, category_id, description) VALUES
  (1, 1, 'Stainless-steel vacuum flask, 0.5 L, for hot or cold beverages'),
  (2, 1, '500 ml plastic sports water bottle, not insulated'),
  (3, 2, 'Bluetooth over-ear headphones with active noise cancelling');
