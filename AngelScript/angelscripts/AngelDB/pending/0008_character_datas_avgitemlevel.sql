-- Add avgitemlevel column to character_datas
-- Stores the player's average equipped item level, updated on logout.
ALTER TABLE character_datas
    ADD COLUMN avgitemlevel FLOAT DEFAULT 0 NOT NULL;
