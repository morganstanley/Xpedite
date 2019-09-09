///////////////////////////////////////////////////////////////////////////////
//
// Custom class transformer to transform Java classes inserting Xpedite probes
//
// Author: Brooke Elizabeth Cantwell, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

package com.xpedite;
import com.xpedite.probes.CallSite;
import javassist.CannotCompileException;
import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtMethod;
import javassist.NotFoundException;

import java.io.IOException;

import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.IllegalClassFormatException;
import java.security.ProtectionDomain;

public class ClassTransformer implements ClassFileTransformer {
    private CallSite callSite;
    public ClassTransformer(CallSite callSite) {
        this.callSite = callSite;
    }

    @Override
    public byte[] transform(ClassLoader loader, String className, Class<?> classBeingRedefined,
                            ProtectionDomain protectionDomain, byte[] classfileBuffer) throws IllegalClassFormatException {
        if (!callSite.getClassName().equals(className)) {
            return classfileBuffer;
        }

        byte[] bytecode = classfileBuffer;
        try {
            CtClass ctClass = ClassPool.getDefault().get(callSite.getClassName().replace('/', '.'));
            CtMethod ctClassMethod = ctClass.getDeclaredMethod(callSite.getMethodName());
            if (ctClassMethod == null) {
                return classfileBuffer;
            }
            if (callSite.getLineNo() == 0) {
                ctClassMethod.insertBefore(buildTrampoline(callSite.getId()));
            } else if (callSite.getLineNo() == Integer.MAX_VALUE) {
                ctClassMethod.insertAfter(buildTrampoline(callSite.getId()), true);
            } else {
                ctClassMethod.insertAt(callSite.getLineNo(), buildTrampoline(callSite.getId()));
            }
            bytecode = ctClass.toBytecode();
            ctClass.defrost();
        } catch (IOException | RuntimeException e) {
            throw new IllegalClassFormatException(e.getMessage());
        } catch (NotFoundException | CannotCompileException e) {
            e.printStackTrace();
        }
        return bytecode;
    }

    private String buildTrampoline(int id) {
        StringBuffer trampoline = new StringBuffer("com.xpedite.Xpedite.record(");
        trampoline.append(id);
        trampoline.append(");");
        return trampoline.toString();
    }
}
