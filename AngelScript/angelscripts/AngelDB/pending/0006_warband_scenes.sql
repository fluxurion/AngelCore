-- AngelDB: Warband Scene Unlocks
-- Tracks which warband scenes each account has unlocked.

CREATE TABLE IF NOT EXISTS `battlenet_account_warband_scenes` (
  `battlenetAccountId` int unsigned NOT NULL,
  `warbandSceneId`     int unsigned NOT NULL,
  `isFavorite`         tinyint unsigned NOT NULL DEFAULT 0,
  `hasFanfare`         tinyint unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`battlenetAccountId`, `warbandSceneId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
