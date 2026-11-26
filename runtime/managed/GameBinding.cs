// SPDX-License-Identifier: MIT
using System;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;

namespace Sora.Runtime
{
    internal sealed class GameBinding
    {
        internal MethodInfo Check { get; }
        internal MethodInfo Start { get; }
        private readonly MethodInfo entityGetter, componentGetter, isRole, playerId, modelGetter, localId;
        internal GameBinding(Module module)
        {
            Check = Resolve(module, 0x06018c9f, "CharacterSkillCdComponent", "IsSkillInCd");
            Start = Resolve(module, 0x06018ca3, "CharacterSkillCdComponent", "StartCd");
            VerifyBody(Check, "6f9a1712cc06c8a16f9b2e187a996f18e6e99986c94f5b57368cc40b8588d005");
            VerifyBody(Start, "2d77a100ff653a099aaa57aaf4fddbb30b710cfec5466cc96fc6d9c8c6cd14a2");
            if (Check.ReturnType != typeof(bool) || Start.ReturnType != typeof(bool) ||
                Check.GetParameters().Length != 2 || Start.GetParameters().Length != 2 ||
                Check.GetParameters()[0].ParameterType != typeof(int) ||
                Check.GetParameters()[1].ParameterType != typeof(bool))
                throw new InvalidOperationException("Unexpected cooldown signature");
            isRole = Resolve(module, 0x0601841b, "CreatureDataComponent", "IsRole");
            playerId = Resolve(module, 0x06018409, "CreatureDataComponent", "GetPlayerId");
            modelGetter = Resolve(module, 0x0601bab7, "CreatureModel", "get_Instance");
            localId = Resolve(module, 0x0601ba5a, "CreatureModel", "GetPlayerId");
            if (playerId.ReturnType != typeof(int) || localId.ReturnType != typeof(int) ||
                isRole.ReturnType != typeof(bool) || !modelGetter.IsStatic)
                throw new InvalidOperationException("Unexpected ownership signature");
            entityGetter = Check.DeclaringType.GetProperty("Entity",
                BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic)?.GetGetMethod(true);
            if (entityGetter == null) throw new InvalidOperationException("No component entity");
            var candidates = entityGetter.ReturnType.GetMethods(BindingFlags.Instance | BindingFlags.Public)
                .Where(m => m.Name == "GetComponent" && m.IsGenericMethodDefinition &&
                    m.GetGenericArguments().Length == 1 && m.GetParameters().Length == 0).ToArray();
            if (candidates.Length != 1) throw new InvalidOperationException("Ambiguous component getter");
            componentGetter = candidates[0].MakeGenericMethod(isRole.DeclaringType);
        }
        private static MethodInfo Resolve(Module module, int token, string type, string name)
        {
            var method = module.ResolveMethod(token) as MethodInfo;
            if (method == null || method.Name != name || method.DeclaringType.FullName != type)
                throw new InvalidOperationException("Unexpected method identity");
            return method;
        }
        private static void VerifyBody(MethodInfo method, string expected)
        {
            var bytes = method.GetMethodBody()?.GetILAsByteArray();
            if (bytes == null) throw new InvalidOperationException("No managed method body");
            using (var hash = SHA256.Create())
                if (BitConverter.ToString(hash.ComputeHash(bytes)).Replace("-", "").ToLowerInvariant() != expected)
                    throw new InvalidOperationException("Method body differs from inspected build");
        }
        internal bool IsLocalRole(object component)
        {
            var entity = entityGetter.Invoke(component, null);
            if (entity == null) return false;
            var creature = componentGetter.Invoke(entity, null);
            if (creature == null || !(bool)isRole.Invoke(creature, null)) return false;
            var id = (int)playerId.Invoke(creature, null);
            if (id <= 0) return false;
            var model = modelGetter.Invoke(null, null);
            return model != null && id == (int)localId.Invoke(model, null);
        }
    }
}
