"""
noar_import_spec.py - designer spec CSV -> Unreal data (Standard v1.1, section 10).

    import noar_import_spec as spec
    spec.import_finish_options("/Game/Apartments/DEMPSTER/Data", "<repo>/Data/Dempster/finish_options.csv")
    spec.import_products("/Game/Apartments/DEMPSTER/Data/Products", "<repo>/Data/Dempster/products.csv")

finish_options.csv -> DT_FinishOptions_<APT> (row struct FNOARFinishOptionRow, defined in NOARCore/NOARTypes.h)
products.csv       -> one DA_Product_<SpecCode> per row (PrimaryDataAsset subclass BP: /Game/NOAR/Configurator/DA_Product)

The CSV is the source of truth: re-running overwrites rows/assets with the same key. Materials and
thumbnails are soft paths resolved later by the artist; missing ones are logged, not fatal.
"""
import csv
import unreal

ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def import_finish_options(dest_folder: str, csv_path: str, table_name: str = "DT_FinishOptions_DEMPSTER"):
    row_struct = unreal.load_object(None, "/Script/NOARCore.NOARFinishOptionRow")
    if not row_struct:
        unreal.log_error("[NOAR] FNOARFinishOptionRow not found - build NOARCore first")
        return None

    table_path = f"{dest_folder}/{table_name}"
    table = unreal.load_asset(table_path)
    if not table:
        factory = unreal.DataTableFactory()
        factory.struct = row_struct
        table = ASSET_TOOLS.create_asset(table_name, dest_folder, unreal.DataTable, factory)

    # DataTable CSV import expects the first column to be the row name
    with open(csv_path, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    header = ["---"] + [k for k in rows[0].keys() if k != "RowName"]
    lines = [",".join(header)]
    for r in rows:
        vals = [r["RowName"]] + [f'"{r[k]}"' for k in header[1:]]
        lines.append(",".join(vals))
    csv_text = "\n".join(lines)

    problems = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, csv_text)
    for p in problems:
        unreal.log_warning(f"[NOAR] {table_name}: {p}")
    unreal.EditorAssetLibrary.save_asset(table_path)
    unreal.log(f"[NOAR] {table_name}: {len(rows)} rows imported")
    return table


def import_products(dest_folder: str, csv_path: str, da_class_path: str = "/Game/NOAR/Configurator/DA_Product"):
    da_class = unreal.EditorAssetLibrary.load_blueprint_class(da_class_path)
    if not da_class:
        unreal.log_error(f"[NOAR] DA_Product class not found at {da_class_path}")
        return []

    created = []
    with open(csv_path, newline="", encoding="utf-8") as f:
        for r in csv.DictReader(f):
            name = f"DA_Product_{r['SpecCode']}"
            path = f"{dest_folder}/{name}"
            asset = unreal.load_asset(path)
            if not asset:
                factory = unreal.DataAssetFactory()
                factory.data_asset_class = da_class
                asset = ASSET_TOOLS.create_asset(name, dest_folder, None, factory)
            info = unreal.NOARProductInfo()
            info.spec_code = r["SpecCode"]
            info.manufacturer = r.get("Manufacturer", "")
            info.model = r.get("Model", "")
            info.material = r.get("Material", "")
            info.finish = r.get("Finish", "")
            info.color = r.get("Color", "")
            try:
                info.price = float(r.get("Price") or 0)
            except ValueError:
                info.price = 0.0
            info.url = r.get("URL", "")
            try:
                asset.set_editor_property("Info", info)
                asset.set_editor_property("Location", r.get("Location", ""))
                asset.set_editor_property("Category", r.get("Category", ""))
            except Exception as e:
                unreal.log_warning(f"[NOAR] {name}: {e}")
            unreal.EditorAssetLibrary.save_asset(path)
            created.append(path)
    unreal.log(f"[NOAR] products: {len(created)} assets written to {dest_folder}")
    return created
