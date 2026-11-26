"""Behavior checks for the injected owner predicate; never loads game code."""
import struct
import unittest
from prepare import guard, TOKENS


def run_guard(code, entity, model):
    stack, pc = [], 0
    while pc < len(code):
        opcode = code[pc]; pc += 1
        if opcode == 0x02:
            stack.append({'entity': entity})
        elif opcode == 0x25:
            stack.append(stack[-1])
        elif opcode == 0x26:
            stack.pop()
        elif opcode in (0x16, 0x17):
            stack.append(opcode - 0x16)
        elif opcode == 0x22:
            stack.append(struct.unpack_from('<f', code, pc)[0]); pc += 4
        elif opcode in (0x28, 0x6f):
            token = struct.unpack_from('<I', code, pc)[0]; pc += 4
            if token == TOKENS['model']:
                stack.append(model)
            else:
                value = stack.pop()
                if token == TOKENS['entity']:
                    stack.append(value['entity'])
                elif token == TOKENS['creature']:
                    stack.append(value.get('creature'))
                elif token == TOKENS['is_role']:
                    stack.append(value['role'])
                elif token in (TOKENS['owner_id'], TOKENS['self_id']):
                    stack.append(value['id'])
                else:
                    raise AssertionError('Unknown call token')
        elif opcode in (0x38, 0x3a, 0x3d, 0x40):
            delta = struct.unpack_from('<i', code, pc)[0]; pc += 4
            if opcode == 0x38:
                take = True
            elif opcode == 0x3a:
                value = stack.pop()
                take = value is not None and (not isinstance(value, (int, bool)) or value != 0)
            else:
                right, left = stack.pop(), stack.pop()
                take = left > right if opcode == 0x3d else left != right
            if take:
                pc += delta
        elif opcode == 0x2a:
            result = stack.pop() if stack else None
            if stack:
                raise AssertionError('Return left extra values on stack')
            return 'return', result
        else:
            raise AssertionError('Unknown opcode')
        if len(stack) > 3:
            raise AssertionError('Exceeded declared guard stack bound')
    if stack:
        raise AssertionError('Fallback must have an empty stack')
    return 'original', None


class GuardTests(unittest.TestCase):
    def test_owner_and_non_owner_paths(self):
        cases = [
            (None, {'id': 7}, 'original'),
            ({}, {'id': 7}, 'original'),
            ({'creature': None}, {'id': 7}, 'original'),
            ({'creature': {'role': False, 'id': 7}}, {'id': 7}, 'original'),
            ({'creature': {'role': True, 'id': 0}}, {'id': 7}, 'original'),
            ({'creature': {'role': True, 'id': -1}}, {'id': 7}, 'original'),
            ({'creature': {'role': True, 'id': 8}}, {'id': 7}, 'original'),
            ({'creature': {'role': True, 'id': 7}}, None, 'original'),
            ({'creature': {'role': True, 'id': 7}}, {'id': 7}, 'return'),
        ]
        returns = [(b'\x16\x2a', 0), (b'\x17\x2a', 1), (b'\x22\0\0\0\0\x2a', 0.0), (b'\x2a', None)]
        for return_code, result in returns:
            for entity, model, action in cases:
                with self.subTest(entity=entity, model=model, return_code=return_code):
                    self.assertEqual(run_guard(guard(return_code), entity, model), (action, result if action == 'return' else None))


if __name__ == '__main__':
    unittest.main()
