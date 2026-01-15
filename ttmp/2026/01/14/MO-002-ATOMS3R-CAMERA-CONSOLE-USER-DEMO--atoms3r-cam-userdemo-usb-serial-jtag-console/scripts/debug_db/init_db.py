#!/usr/bin/env python3
import argparse
import pathlib
import sqlite3


def load_schema(schema_path: pathlib.Path) -> str:
    return schema_path.read_text(encoding="utf-8")


def init_db(db_path: pathlib.Path, schema_path: pathlib.Path) -> None:
    db_path.parent.mkdir(parents=True, exist_ok=True)
    with sqlite3.connect(db_path) as conn:
        conn.executescript(load_schema(schema_path))


def main() -> None:
    parser = argparse.ArgumentParser(description="Initialize the 0041 debugging sqlite database")
    parser.add_argument("--db", required=True, help="Path to sqlite db")
    parser.add_argument(
        "--schema",
        default=str(pathlib.Path(__file__).with_name("schema.sql")),
        help="Path to schema.sql",
    )
    args = parser.parse_args()

    db_path = pathlib.Path(args.db)
    schema_path = pathlib.Path(args.schema)
    init_db(db_path, schema_path)
    print(f"Initialized db: {db_path}")


if __name__ == "__main__":
    main()
