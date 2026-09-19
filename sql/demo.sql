USE sakila;

-- Bound the candidate set before asking Jev. LIMIT alone does not bound calls.
SELECT film_id, title, description
FROM film
WHERE film_id BETWEEN 1 AND 8
  AND description AILIKE 'The story is set somewhere in Asia.';

SELECT film_id, title, description
FROM film
WHERE film_id BETWEEN 1 AND 8
  AND description AILIKE 'The story features somebody whose profession is preparing food.';
