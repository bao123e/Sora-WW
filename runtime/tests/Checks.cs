// SPDX-License-Identifier: MIT
using System;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Threading;
using HarmonyLib;
using Sora.Runtime;

class Actor
{
    internal int Owner;
    internal bool Role;
    internal bool Disposed;
    internal int OriginalChecks, OriginalStarts;
    [MethodImpl(MethodImplOptions.NoInlining)]
    public bool IsSkillInCd(int skill, bool remaining) { OriginalChecks++; return true; }
    [MethodImpl(MethodImplOptions.NoInlining)]
    public bool StartCd(int skill, int? section) { OriginalStarts++; return false; }
}

static class Checks
{
    static NoCooldown feature;
    static bool CheckPrefix(object __instance, ref bool __result) => feature.InCooldown(__instance, ref __result);
    static bool StartPrefix(object __instance, ref bool __result) => feature.Start(__instance, ref __result);
    static void Require(bool value, string message) { if (!value) throw new Exception(message); }
    static void Main()
    {
        feature = new NoCooldown(value => {
            var actor = (Actor)value;
            if (actor.Disposed) throw new ObjectDisposedException("actor");
            return actor.Role && actor.Owner > 0 && actor.Owner == 7;
        });
        var harmony = new Harmony("SoraWW.Fixture.Only");
        try
        {
            harmony.Patch(typeof(Actor).GetMethod("IsSkillInCd"), prefix: new HarmonyMethod(typeof(Checks).GetMethod(nameof(CheckPrefix), BindingFlags.Static | BindingFlags.NonPublic)));
            harmony.Patch(typeof(Actor).GetMethod("StartCd"), prefix: new HarmonyMethod(typeof(Checks).GetMethod(nameof(StartPrefix), BindingFlags.Static | BindingFlags.NonPublic)));
            var local = new Actor {Owner=7,Role=true};
            Require(local.IsSkillInCd(1,true) && !local.StartCd(1,null), "Disabled hooks changed behavior");
            feature.Set(true);
            Require(!local.IsSkillInCd(1,true) && local.StartCd(1,null), "Enabled local hooks did not intercept");
            Require(local.OriginalChecks==1 && local.OriginalStarts==1, "Intercept ran original method");
            foreach (var other in new[] {new Actor{Owner=8,Role=true},new Actor{Owner=7,Role=false},new Actor{Owner=0,Role=true},new Actor{Owner=7,Role=true,Disposed=true}})
                Require(other.IsSkillInCd(1,true) && !other.StartCd(1,null), "Remote/nonrole/disposed entity was modified");
            feature.Set(false);
            Require(local.IsSkillInCd(1,true) && !local.StartCd(1,null), "OFF did not restore original method");
            feature.Set(true);
            Require(!local.IsSkillInCd(1,true), "Re-enable failed");
            Thread.Sleep(3250); // exercise real monotonic lease after lost controller
            Require(local.IsSkillInCd(1,true) && !local.StartCd(1,null), "Lost controller did not revert behavior");
            Require(Entry.Execute("OFF")=="OFF", "OFF requires an initialized game");
            Require(Entry.Execute("ON")=="GAME_ASSEMBLY_NOT_LOADED", "Missing actual game was falsely reported as active");
            Console.WriteLine("PASS actual Harmony detours: local/remote/nonrole/disposed, OFF/re-enable, lost-controller lease, missing game rejection");
        }
        finally { feature.Set(false); harmony.UnpatchAll("SoraWW.Fixture.Only"); }
    }
}
