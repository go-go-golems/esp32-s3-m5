import { logI32 } from "../../shared/host";

export function run(): i32 {
  logI32(9, 42);
  return 42;
}
