import os
import logging
from datetime import datetime
from sqlalchemy.ext.asyncio import create_async_engine, AsyncSession
from sqlalchemy.orm import declarative_base, sessionmaker
from sqlalchemy import Column, Integer, Float, String, Boolean, DateTime, Text, ForeignKey

# Setup database logger
logger = logging.getLogger("rai_web_api.database")

# DB URLs: Default to PostgreSQL but gracefully fallback to SQLite if needed
DATABASE_URL_PG = os.getenv(
    "DATABASE_URL",
    "postgresql+asyncpg://postgres:postgres@localhost:5432/rai_db"
)
# Local fallback SQLite path
SQLITE_DB_PATH = os.path.expanduser("~/rai_datasets/rai_web.db")
DATABASE_URL_SQLITE = f"sqlite+aiosqlite:///{SQLITE_DB_PATH}"

Base = declarative_base()

# --- MODELS ---

class RobotConfig(Base):
    __tablename__ = 'robot_configs'

    id = Column(Integer, primary_key=True)
    robot_name = Column(String(50), nullable=False, default='rai_robot')
    max_linear_x = Column(Float, nullable=False, default=0.5)
    max_linear_y = Column(Float, nullable=False, default=0.5)
    max_angular_z = Column(Float, nullable=False, default=1.0)
    battery_min_v = Column(Float, nullable=False, default=20.0)
    battery_max_v = Column(Float, nullable=False, default=25.2)
    camera_fps = Column(Integer, nullable=False, default=15)
    active_map = Column(String(100), nullable=True)
    is_slam_active = Column(Boolean, nullable=False, default=False)
    updated_at = Column(DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

class DatasetScenario(Base):
    __tablename__ = 'dataset_scenarios'

    id = Column(Integer, primary_key=True)
    name = Column(String(100), unique=True, nullable=False)
    context_type = Column(String(30), nullable=False, default='CONTINUOUS')
    difficulty = Column(String(20), nullable=False, default='medium')
    human_mode = Column(String(50), nullable=True)
    expected_runs = Column(Integer, nullable=False, default=20)
    description = Column(Text, nullable=True)
    created_at = Column(DateTime, default=datetime.utcnow)

class DatasetRun(Base):
    __tablename__ = 'dataset_runs'

    id = Column(Integer, primary_key=True)
    scenario_id = Column(Integer, ForeignKey('dataset_scenarios.id', ondelete='SET NULL'), nullable=True)
    run_name = Column(String(150), unique=True, nullable=False)
    environment = Column(String(20), nullable=False, default='real')
    controller_id = Column(String(50), nullable=False, default='CCA_NMPC')
    run_index = Column(Integer, nullable=True)
    split = Column(String(20), nullable=False, default='unsplit')
    data_path = Column(String(250), nullable=False)
    raw_bag_path = Column(String(250), nullable=True)
    metadata_path = Column(String(250), nullable=True)
    zip_path = Column(String(250), nullable=True)
    status = Column(String(20), nullable=False) # RECORDING, COMPLETED, FAILED, COMPRESSED
    validation_status = Column(String(20), nullable=True)
    success = Column(Boolean, nullable=True)
    start_time = Column(DateTime, nullable=False, default=datetime.utcnow)
    end_time = Column(DateTime, nullable=True)
    duration = Column(Float, nullable=True)
    samples_count = Column(Integer, default=0)
    phi_h_max = Column(Float, nullable=True)
    min_human_clearance = Column(Float, nullable=True)
    timeout_rate = Column(Float, nullable=True)
    rmse_xy = Column(Float, nullable=True)
    rmse_theta = Column(Float, nullable=True)
    max_lateral_error = Column(Float, nullable=True)
    d_min = Column(Float, nullable=True)
    d_avg = Column(Float, nullable=True)
    d_5percentile = Column(Float, nullable=True)
    violation_count = Column(Integer, nullable=True)
    violation_duration = Column(Float, nullable=True)
    collision_count = Column(Integer, nullable=True)
    jerk_mean = Column(Float, nullable=True)
    jerk_max = Column(Float, nullable=True)
    mean_abs_delta_u = Column(Float, nullable=True)
    max_abs_delta_u = Column(Float, nullable=True)
    control_effort = Column(Float, nullable=True)
    mean_abs_vx = Column(Float, nullable=True)
    mean_abs_vy = Column(Float, nullable=True)
    mean_abs_omega = Column(Float, nullable=True)
    solve_time_mean_ms = Column(Float, nullable=True)
    solve_time_median_ms = Column(Float, nullable=True)
    solve_time_p95_ms = Column(Float, nullable=True)
    solve_time_max_ms = Column(Float, nullable=True)
    start_voltage = Column(Float, nullable=True)
    end_voltage = Column(Float, nullable=True)
    min_voltage = Column(Float, nullable=True)
    avg_voltage = Column(Float, nullable=True)
    min_percentage = Column(Float, nullable=True)
    avg_percentage = Column(Float, nullable=True)
    file_size_bytes = Column(Integer, default=0)
    notes = Column(Text, nullable=True)

class NavWaypoint(Base):
    __tablename__ = 'nav_waypoints'

    id = Column(Integer, primary_key=True)
    label = Column(String(50), unique=True, nullable=False)
    x = Column(Float, nullable=False)
    y = Column(Float, nullable=False)
    yaw = Column(Float, nullable=False)
    description = Column(Text, nullable=True)
    created_at = Column(DateTime, default=datetime.utcnow)

class SystemLog(Base):
    __tablename__ = 'system_logs'

    id = Column(Integer, primary_key=True)
    timestamp = Column(DateTime, default=datetime.utcnow)
    level = Column(String(10), nullable=False)
    component = Column(String(50), nullable=False)
    message = Column(Text, nullable=False)

# --- DATABASE ENGINE & SESSION MANAGERS ---

_engine = None
_SessionLocal = None

async def init_db():
    global _engine, _SessionLocal

    # Try PostgreSQL first
    try:
        logger.info("Connecting to PostgreSQL database...")
        _engine = create_async_engine(DATABASE_URL_PG, echo=False)
        # Verify connection by running a dummy select
        async with _engine.begin() as conn:
            await conn.run_sync(Base.metadata.create_all)
        logger.info("Successfully connected and initialized PostgreSQL!")
    except Exception as e:
        logger.warning(f"PostgreSQL connection failed: {e}. Falling back to SQLite...")

        # Fallback to local SQLite
        os.makedirs(os.path.dirname(SQLITE_DB_PATH), exist_ok=True)
        _engine = create_async_engine(DATABASE_URL_SQLITE, echo=False)
        logger.info(f"SQLite DB path: {SQLITE_DB_PATH}")

    # Bind sessionmaker
    _SessionLocal = sessionmaker(
        _engine, expire_on_commit=False, class_=AsyncSession
    )

    # Make tables and seed initial data
    async with _engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)
        await _ensure_schema_columns(conn)

    await seed_initial_data()


async def _ensure_schema_columns(conn):
    """Best-effort additive schema update for existing SQLite/PostgreSQL DBs."""
    dialect = conn.dialect.name
    table_columns = {
        'dataset_scenarios': [
            ('human_mode', 'VARCHAR(50)'),
            ('expected_runs', 'INTEGER DEFAULT 20'),
        ],
        'dataset_runs': [
            ('environment', "VARCHAR(20) DEFAULT 'real'"),
            ('controller_id', "VARCHAR(50) DEFAULT 'CCA_NMPC'"),
            ('run_index', 'INTEGER'),
            ('split', "VARCHAR(20) DEFAULT 'unsplit'"),
            ('raw_bag_path', 'VARCHAR(250)'),
            ('metadata_path', 'VARCHAR(250)'),
            ('validation_status', 'VARCHAR(20)'),
            ('success', 'BOOLEAN'),
            ('phi_h_max', 'FLOAT'),
            ('min_human_clearance', 'FLOAT'),
            ('timeout_rate', 'FLOAT'),
            ('rmse_xy', 'FLOAT'),
            ('rmse_theta', 'FLOAT'),
            ('max_lateral_error', 'FLOAT'),
            ('d_min', 'FLOAT'),
            ('d_avg', 'FLOAT'),
            ('d_5percentile', 'FLOAT'),
            ('violation_count', 'INTEGER'),
            ('violation_duration', 'FLOAT'),
            ('collision_count', 'INTEGER'),
            ('jerk_mean', 'FLOAT'),
            ('jerk_max', 'FLOAT'),
            ('mean_abs_delta_u', 'FLOAT'),
            ('max_abs_delta_u', 'FLOAT'),
            ('control_effort', 'FLOAT'),
            ('mean_abs_vx', 'FLOAT'),
            ('mean_abs_vy', 'FLOAT'),
            ('mean_abs_omega', 'FLOAT'),
            ('solve_time_mean_ms', 'FLOAT'),
            ('solve_time_median_ms', 'FLOAT'),
            ('solve_time_p95_ms', 'FLOAT'),
            ('solve_time_max_ms', 'FLOAT'),
        ],
    }

    for table, columns in table_columns.items():
        existing_columns = set()
        if dialect == 'sqlite':
            result = await conn.exec_driver_sql(f'PRAGMA table_info({table})')
            existing_columns = {row[1] for row in result.fetchall()}

        for column_name, column_type in columns:
            if dialect == 'sqlite' and column_name in existing_columns:
                continue
            try:
                if dialect == 'postgresql':
                    await conn.exec_driver_sql(
                        f'ALTER TABLE {table} ADD COLUMN IF NOT EXISTS {column_name} {column_type}'
                    )
                else:
                    await conn.exec_driver_sql(
                        f'ALTER TABLE {table} ADD COLUMN {column_name} {column_type}'
                    )
            except Exception:
                # Best effort for local development databases.
                pass

async def get_db():
    if _SessionLocal is None:
        raise RuntimeError("Database not initialized. Call init_db() first.")
    async with _SessionLocal() as session:
        try:
            yield session
        finally:
            await session.close()


def create_session() -> AsyncSession:
    """Tạo session độc lập dùng trong background tasks (không dùng `async with`)."""
    if _SessionLocal is None:
        raise RuntimeError("Database not initialized. Call init_db() first.")
    return _SessionLocal()

async def seed_initial_data():
    """Seed initial robot configuration and scenarios if tables are empty"""
    from sqlalchemy import select
    async with _SessionLocal() as session:
        # Check config
        result = await session.execute(select(RobotConfig))
        if not result.scalars().first():
            config = RobotConfig(
                robot_name="rai_robot",
                max_linear_x=0.5,
                max_linear_y=0.5,
                max_angular_z=1.0,
                battery_min_v=20.0,
                battery_max_v=25.2,
                camera_fps=15,
                is_slam_active=False
            )
            session.add(config)
            logger.info("Seeded default robot configuration.")

        scenario_specs = [
            DatasetScenario(
                name="S1_open_zone",
                context_type="CONTINUOUS",
                difficulty="easy",
                human_mode="none",
                expected_runs=20,
                description="Manual open-zone protocol; no close human, low phi_h expected.",
            ),
            DatasetScenario(
                name="S2_narrow_corridor",
                context_type="CONTINUOUS",
                difficulty="medium",
                human_mode="none",
                expected_runs=20,
                description="Manual narrow-corridor/aisle protocol; collect clearance and lateral Mecanum behavior.",
            ),
            DatasetScenario(
                name="S3_human_proximate",
                context_type="CONTINUOUS",
                difficulty="hard",
                human_mode="static_or_slow",
                expected_runs=20,
                description="Manual human-proximate protocol; collect continuous safety adaptation near a person.",
            ),
            DatasetScenario(
                name="S4_dynamic_crossing",
                context_type="CONTINUOUS",
                difficulty="hard",
                human_mode="dynamic_crossing",
                expected_runs=20,
                description="Manual dynamic crossing protocol; collect human velocity estimation and robot response.",
            ),
            DatasetScenario(
                name="S5_occlusion_sudden_appearance",
                context_type="CONTINUOUS",
                difficulty="stress",
                human_mode="sudden_appearance",
                expected_runs=20,
                description="Manual occlusion/sudden-appearance protocol; collect reaction and fallback behavior.",
            ),
        ]

        for scenario in scenario_specs:
            result = await session.execute(select(DatasetScenario).where(DatasetScenario.name == scenario.name))
            if not result.scalars().first():
                session.add(scenario)
        logger.info("Ensured 5 manual S1-S5 CCA-NMPC dataset scenarios exist.")

        await session.commit()
