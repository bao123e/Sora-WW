// SPDX-License-Identifier: MIT
using System;
using System.Diagnostics;
using System.Reflection;
using System.Threading;
using HarmonyLib;

namespace Sora.Runtime
{
    // The clock lease is independent of the controller's UI state. If the menu,
    // pipe, helper or worker disappears, prefixes resume original behavior.
    internal sealed class Lease
    {
        private long deadline;
        public bool Active => Stopwatch.GetTimestamp() < Interlocked.Read(ref deadline);
        public void Renew() => Interlocked.Exchange(ref deadline,
            Stopwatch.GetTimestamp() + Stopwatch.Frequency * 3);
        public void Clear() => Interlocked.Exchange(ref deadline, 0);
    }

    internal sealed class NoCooldown
    {
        private readonly Lease lease = new Lease();
        private readonly Func<object, bool> isLocalRole;
        internal long ScopedChecks, BypassedChecks, BypassedStarts;
        internal NoCooldown(Func<object, bool> predicate) { isLocalRole = predicate; }
        internal void Set(bool enabled) { if (enabled) lease.Renew(); else lease.Clear(); }
        private bool Applies(object component)
        {
            if (!lease.Active || component == null) return false;
            try
            {
                if (!isLocalRole(component)) return false;
                Interlocked.Increment(ref ScopedChecks);
                return true;
            }
            catch
            {
                // A disposed/changing entity must still take the game path.
                return false;
            }
        }
        internal bool InCooldown(object component, ref bool result)
        {
            if (!Applies(component)) return true;
            result = false;
            Interlocked.Increment(ref BypassedChecks);
            return false;
        }
        internal bool Start(object component, ref bool result)
        {
            if (!Applies(component)) return true;
            result = true;
            Interlocked.Increment(ref BypassedStarts);
            return false;
        }
    }

    // Named prefixes take no dependency on the game assembly at compile time.
    // Parameters remain untouched and all non-local instances execute normally.
    public static class Entry
    {
        private const string Owner = "SoraWW.LiveNoCooldown.1";
        private static readonly object Gate = new object();
        private static NoCooldown feature;
        private static string failure;
        private static bool PrefixCheck(object __instance, ref bool __result) =>
            feature == null || feature.InCooldown(__instance, ref __result);
        private static bool PrefixStart(object __instance, ref bool __result) =>
            feature == null || feature.Start(__instance, ref __result);

        // Native embedding passes one small command and consumes a status string.
        // No network, game file writes, account text or screenshots.
        public static string Execute(string command)
        {
            lock (Gate)
            {
                if (command == "OFF")
                {
                    feature?.Set(false);
                    return "OFF";
                }
                if (command != "ON") return "INVALID_COMMAND";
                if (failure != null) return failure;
                if (feature == null)
                {
                    var assembly = Array.Find(AppDomain.CurrentDomain.GetAssemblies(),
                        a => a.GetName().Name == "Aki.Game");
                    if (assembly == null) return "GAME_ASSEMBLY_NOT_LOADED";
                    if (assembly.ManifestModule.ModuleVersionId !=
                        new Guid("2ea7a717-61c0-4953-9f6c-13b1ec3564cd"))
                        return "UNSUPPORTED_GAME_ASSEMBLY";
                    var harmony = new Harmony(Owner);
                    try
                    {
                        var binding = new GameBinding(assembly.ManifestModule);
                        // Install while disabled. Both hooks must succeed before ON.
                        feature = new NoCooldown(binding.IsLocalRole);
                        harmony.Patch(binding.Check, prefix: new HarmonyMethod(
                            typeof(Entry).GetMethod(nameof(PrefixCheck), BindingFlags.NonPublic | BindingFlags.Static)));
                        harmony.Patch(binding.Start, prefix: new HarmonyMethod(
                            typeof(Entry).GetMethod(nameof(PrefixStart), BindingFlags.NonPublic | BindingFlags.Static)));
                    }
                    catch
                    {
                        feature?.Set(false);
                        try { harmony.UnpatchAll(Owner); } catch { }
                        failure = "HOOK_INSTALL_FAILED";
                        return failure;
                    }
                }
                feature.Set(true);
                // HOOKS_READY is deliberately distinct from observed gameplay.
                return "HOOKS_READY:" + Interlocked.Read(ref feature.BypassedChecks) + ":" +
                    Interlocked.Read(ref feature.BypassedStarts);
            }
        }
    }
}
