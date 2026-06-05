-- ============================================================================
-- AngelCore Spawn System — Creature & GameObject Persistence
-- ============================================================================
-- Stores AS-spawned creatures & gameobjects in angelcore_db.
-- Completely separate from TC's world.creature / world.gameobject tables.
-- GUIDs use the AS-specific range (bit 39 set) to avoid collision with TC.
-- ============================================================================

-- ----------------------------------------------------------------------------
-- Creature Spawns
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `as_creature_spawns` (
  `guid`          BIGINT UNSIGNED NOT NULL COMMENT 'AS spawn GUID (bit 39 set)',
  `entry`         INT UNSIGNED NOT NULL COMMENT 'creature_template entry',
  `map`           SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `position_x`    FLOAT NOT NULL DEFAULT 0,
  `position_y`    FLOAT NOT NULL DEFAULT 0,
  `position_z`    FLOAT NOT NULL DEFAULT 0,
  `orientation`   FLOAT NOT NULL DEFAULT 0,
  `phaseId`       INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = all phases',
  `respawnDelaySecs` INT UNSIGNED NOT NULL DEFAULT 0,
  `spawntimesecs` INT UNSIGNED NOT NULL DEFAULT 120,
  `wander_distance` FLOAT NOT NULL DEFAULT 0,
  `movementType`  TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=idle, 1=random, 2=waypoint',
  `npcflag`       BIGINT UNSIGNED DEFAULT NULL,
  `unit_flags`    INT UNSIGNED DEFAULT NULL,
  `unit_flags2`   INT UNSIGNED DEFAULT NULL,
  `faction`       INT UNSIGNED NOT NULL DEFAULT 0,
  `level`         TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = template default',
  `equipment_id`  TINYINT NOT NULL DEFAULT 0,
  `gossipMenuId`  INT UNSIGNED NOT NULL DEFAULT 0,
  `reactState`    TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=passive, 1=defensive, 2=aggressive',
  `display_id`    INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = template default',
  `script_name`   VARCHAR(64) NOT NULL DEFAULT '',
  `string_id`     VARCHAR(64) DEFAULT NULL,
  `is_active`     TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `created_at`    TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `updated_at`    TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`guid`),
  KEY `idx_entry` (`entry`),
  KEY `idx_map`   (`map`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='AngelCore creature spawns';

-- ----------------------------------------------------------------------------
-- GameObject Spawns
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `as_gameobject_spawns` (
  `guid`          BIGINT UNSIGNED NOT NULL COMMENT 'AS spawn GUID (bit 39 set)',
  `entry`         INT UNSIGNED NOT NULL COMMENT 'gameobject_template entry',
  `map`           SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `position_x`    FLOAT NOT NULL DEFAULT 0,
  `position_y`    FLOAT NOT NULL DEFAULT 0,
  `position_z`    FLOAT NOT NULL DEFAULT 0,
  `orientation`   FLOAT NOT NULL DEFAULT 0,
  `rotation0`     FLOAT NOT NULL DEFAULT 0,
  `rotation1`     FLOAT NOT NULL DEFAULT 0,
  `rotation2`     FLOAT NOT NULL DEFAULT 0,
  `rotation3`     FLOAT NOT NULL DEFAULT 1,
  `phaseId`       INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = all phases',
  `respawnDelaySecs` INT UNSIGNED NOT NULL DEFAULT 0,
  `spawntimesecs` INT NOT NULL DEFAULT 0,
  `goState`       TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'GOState: 0=active, 1=ready, 2=destroyed',
  `animprogress`  TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `artKit`        INT UNSIGNED NOT NULL DEFAULT 0,
  `script_name`   VARCHAR(64) NOT NULL DEFAULT '',
  `string_id`     VARCHAR(64) DEFAULT NULL,
  `is_active`     TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `created_at`    TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `updated_at`    TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`guid`),
  KEY `idx_entry` (`entry`),
  KEY `idx_map`   (`map`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='AngelCore gameobject spawns';

-- ----------------------------------------------------------------------------
-- Spawn Config — track next available GUID for AS spawns
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `as_spawn_config` (
  `key`           VARCHAR(64) NOT NULL,
  `value`         BIGINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='AngelCore spawn system config';

-- Initialize the AS spawn GUID counter (bit 39 set = 0x8000000000)
INSERT IGNORE INTO `as_spawn_config` (`key`, `value`) VALUES
  ('next_creature_spawn_id', 0x8000000001),
  ('next_gameobject_spawn_id', 0x8000000001);
