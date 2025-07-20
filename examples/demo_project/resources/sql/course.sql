<?xml version="1.0" encoding="UTF-8"?>
<mapper namespace="Course">
    <!-- 基础查询 -->
    <select id="findAll" resultType="QVariantMap">
        SELECT id, course_code, name, description, credits, hours, teacher, 
               department, semester, max_students, status, created_at, updated_at 
        FROM courses 
        WHERE status = 'ACTIVE'
        ORDER BY course_code
    </select>

    <select id="findById" resultType="QVariantMap">
        SELECT id, course_code, name, description, credits, hours, teacher, 
               department, semester, max_students, status, created_at, updated_at 
        FROM courses 
        WHERE id = :arg1
    </select>

    <select id="findByCourseCode" resultType="QVariantMap">
        SELECT id, course_code, name, description, credits, hours, teacher, 
               department, semester, max_students, status, created_at, updated_at 
        FROM courses 
        WHERE course_code = :arg1
    </select>

    <select id="existsByCourseCode" resultType="int">
        SELECT COUNT(*) FROM courses WHERE course_code = :arg1
    </select>

    <select id="findByDepartment" resultType="QVariantMap">
        SELECT id, course_code, name, description, credits, hours, teacher, 
               department, semester, max_students, status, created_at, updated_at 
        FROM courses 
        WHERE department LIKE :arg1 AND status = 'ACTIVE'
        ORDER BY course_code
    </select>

    <select id="findByTeacher" resultType="QVariantMap">
        SELECT id, course_code, name, description, credits, hours, teacher, 
               department, semester, max_students, status, created_at, updated_at 
        FROM courses 
        WHERE teacher LIKE :arg1 AND status = 'ACTIVE'
        ORDER BY course_code
    </select>

    <select id="canEnrollMore" resultType="int">
        SELECT CASE WHEN COUNT(e.id) &lt; c.max_students THEN 1 ELSE 0 END
        FROM courses c
        LEFT JOIN enrollments e ON c.id = e.course_id AND e.status = 'ENROLLED'
        WHERE c.id = :arg1
        GROUP BY c.id, c.max_students
    </select>

    <select id="count" resultType="int">
        SELECT COUNT(*) FROM courses WHERE status = 'ACTIVE'
    </select>

    <!-- 插入操作 -->
    <insert id="insert">
        INSERT INTO courses (course_code, name, description, credits, hours, teacher, 
                           department, semester, max_students, status)
        VALUES (:course_code, :name, :description, :credits, :hours, :teacher, 
                :department, :semester, :max_students, :status)
    </insert>

    <!-- 更新操作 -->
    <update id="update">
        UPDATE courses 
        SET name = :name, description = :description, credits = :credits, 
            hours = :hours, teacher = :teacher, department = :department, 
            semester = :semester, max_students = :max_students, 
            updated_at = CURRENT_TIMESTAMP
        WHERE id = :id
    </update>

    <update id="updateByCourseCode">
        UPDATE courses 
        SET name = :name, description = :description, credits = :credits, 
            hours = :hours, teacher = :teacher, department = :department, 
            semester = :semester, max_students = :max_students, 
            updated_at = CURRENT_TIMESTAMP
        WHERE course_code = :course_code
    </update>

    <!-- 删除操作 -->
    <delete id="deleteById">
        DELETE FROM courses WHERE id = :arg1
    </delete>

    <update id="softDeleteById">
        UPDATE courses SET status = 'INACTIVE', updated_at = CURRENT_TIMESTAMP 
        WHERE id = :arg1
    </update>
</mapper> 